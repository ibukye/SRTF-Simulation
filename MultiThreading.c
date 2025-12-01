#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Constants
#define MAX_LOG_SIZE 1000
#define COL_WIDTH_PID 12
#define COL_WIDTH_STAT 14
#define COL_WIDTH_REM 16

// State definition using Enum for type safety
typedef enum { STATE_READY, STATE_RUNNING, STATE_COMPLETED } process_state_t;

// Forward declaration
typedef struct SimulationContext SimulationContext;

// Process structure
// NOTE: This structure contains synchronization primitives (pthread_cond_t).
// Therefore, instances of this structure MUST NOT be copied.
// They should be allocated once and passed by pointer.
typedef struct {
  int ID;
  int ArrivalTime;
  int BurstTime;
  int RemainingBurstTime;
  int CompletionTime;
  process_state_t status;
  int TurnaroundTime;
  int WaitingTime;
  int ResponseTime;

  // Threading resources
  pthread_t thread;
  pthread_cond_t my_cond;
  int run_permission; // 1 = allowed to run, 0 = wait

  // Reference to context
  SimulationContext *ctx;
} Process;

// Log structure
typedef struct {
  int time;
  int process_id; // -1 for idle
  int remaining_time;
  process_state_t status;
} GanttLog;

// Simulation Context to encapsulate shared state
struct SimulationContext {
  pthread_mutex_t global_mutex;
  pthread_cond_t scheduler_cond;
  int tick_completed;
  int simulation_finished;
  GanttLog log_buffer[MAX_LOG_SIZE];
  int log_index;
};

// Helper to check pthread return values
void check_pthread(int err, const char *msg) {
  if (err != 0) {
    fprintf(stderr, "Error: %s: %s\n", msg, strerror(err));
    exit(EXIT_FAILURE);
  }
}

// Helper to convert state to string
const char *status_to_string(process_state_t status) {
  switch (status) {
  case STATE_READY:
    return "READY";
  case STATE_RUNNING:
    return "RUNNING";
  case STATE_COMPLETED:
    return "COMPLETED";
  default:
    fprintf(stderr, "Error: Unknown process state %d\n", status);
    exit(EXIT_FAILURE);
  }
}

// Thread function
// DESIGN NOTE: Lock-Step Synchronization
// This simulation uses a "lock-step" approach where the scheduler and worker
// threads strictly alternate execution for each time unit (tick). While this
// limits parallelism (only one thread runs at a time), it is intentional to
// ensure deterministic simulation of the SRTF algorithm and accurate time
// tracking. A true parallel execution would make it difficult to guarantee
// exact 1-second ticks.
void *process_thread_function(void *arg) {
  Process *p = (Process *)arg;
  SimulationContext *ctx = p->ctx;

  while (1) {
    // lock
    check_pthread(pthread_mutex_lock(&ctx->global_mutex),
                  "Lock mutex in thread");

    // until the process turn into runnable, wait
    while (p->run_permission == 0 && !ctx->simulation_finished) {
      check_pthread(pthread_cond_wait(&p->my_cond, &ctx->global_mutex),
                    "Wait cond in thread");
    }

    // if simulation is done
    if (ctx->simulation_finished) {
      check_pthread(pthread_mutex_unlock(&ctx->global_mutex),
                    "Unlock mutex in thread (finish)");
      break;
    }

    // Critical Section
    if (p->RemainingBurstTime > 0) {
      p->RemainingBurstTime--;
    }

    // notify to the scheduler
    p->run_permission = 0;
    ctx->tick_completed = 1;
    check_pthread(pthread_cond_signal(&ctx->scheduler_cond),
                  "Signal scheduler");

    // unlock
    check_pthread(pthread_mutex_unlock(&ctx->global_mutex),
                  "Unlock mutex in thread");

    // if done get out from thread loop
    if (p->RemainingBurstTime == 0) {
      break;
    }
  }
  return NULL;
}

// Helper to read integer input safely
int read_int(const char *prompt, int min, int max) {
  int value;
  char buffer[100];

  while (1) {
    printf("%s", prompt);
    if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
      char *endptr;
      value = strtol(buffer, &endptr, 10);

      if (endptr == buffer || *endptr != '\n') {
        printf("Invalid input. Please enter a number.\n");
        if (processes[i].RemainingBurstTime > 0) {
          // find the shortest remaining time
          if (processes[i].RemainingBurstTime < min_remaining_time) {
            // Update the SRT
            min_remaining_time = processes[i].RemainingBurstTime;
            // Update the index
            shortest_index = i;
          }
        }
      }
    }
    return shortest_index;
  }

  /*
  |  P1  |  P2  |  P3  |...
  0      2      4      5...
  */
  void gantt_chart(SimulationContext * ctx) {
    printf("\n\n--- Visual Gantt Chart ---\n\n");

    if (ctx->log_index == 0)
      return;

    int current_pid = ctx->log_buffer[0].process_id;

    // print the 1st row
    for (int i = 1; i < ctx->log_index; i++) {
      if (ctx->log_buffer[i].status == STATE_COMPLETED)
        continue; // if COMPLETED, just skip

      // if the process ID has changed
      if (ctx->log_buffer[i].process_id != current_pid) {
        if (current_pid == -1) {
          printf("|  IDLE "); // 7 char
        } else {
          printf("|  P%-4d", current_pid); // 7char: "| P1   "
        }
        current_pid = ctx->log_buffer[i].process_id;
      }
    }

    // fin loop
    // print the last block
    if (current_pid == -1) {
      printf("|  IDLE |\n");
    } else {
      printf("|  P%-4d|\n", current_pid);
    }

    // print the time
    current_pid = ctx->log_buffer[0].process_id;
    printf("%-8d", 0); // print the initial time(0)

    for (int i = 1; i < ctx->log_index; i++) {
      if (ctx->log_buffer[i].status == STATE_COMPLETED)
        continue;

      if (ctx->log_buffer[i].process_id != current_pid) {
        // if the curernt process had changed
        // print the time for new process
        printf("%-8d", ctx->log_buffer[i].time); // 7char
        current_pid = ctx->log_buffer[i].process_id;
      }
    }

    // find the last completion time
    int final_time = 0;
    for (int i = ctx->log_index - 1; i >= 0; i--) {
      if (ctx->log_buffer[i].status == STATE_COMPLETED &&
          ctx->log_buffer[i].time > final_time) {
        final_time = ctx->log_buffer[i].time;
      }
    }
    if (final_time == 0 && ctx->log_index > 0)
      final_time = ctx->log_buffer[ctx->log_index - 1].time + 1;

    printf("%-7d\n", final_time);
  }

  // Helper to print a row in the progression table
  void print_row(int time, const char *pid_str, const char *status_str,
                 const char *rem_str) {
    int w_pid = COL_WIDTH_PID;
    int w_stat = COL_WIDTH_STAT;
    int w_rem = COL_WIDTH_REM;

    int l_pid = (w_pid - (int)strlen(pid_str)) / 2;
    int r_pid = w_pid - (int)strlen(pid_str) - l_pid;

    int l_stat = (w_stat - (int)strlen(status_str)) / 2;
    int r_stat = w_stat - (int)strlen(status_str) - l_stat;

    int l_rem = (w_rem - (int)strlen(rem_str)) / 2;
    int r_rem = w_rem - (int)strlen(rem_str) - l_rem;

    printf("%5d |%*s%s%*s|%*s%s%*s|%*s%s%*s|\n", time, l_pid, "", pid_str,
           r_pid, "", l_stat, "", status_str, r_stat, "", l_rem, "", rem_str,
           r_rem, "");
  }

  // Helper to log state
  void add_log(SimulationContext * ctx, int time, int pid, int remaining,
               process_state_t status) {
    if (ctx->log_index >= MAX_LOG_SIZE) {
      fprintf(stderr, "Error: Log buffer overflow. Max size is %d.\n",
              MAX_LOG_SIZE);
      exit(EXIT_FAILURE);
      // create thread
      for (int i = 0; i < num_processes; i++) {
        if (pthread_create(&processes[i].thread, NULL, process_thread_function,
                           &processes[i]) != 0) {
          fprintf(stderr, "Failed to create thread for process %d\n",
                  processes[i].ID);
          goto cleanup;
        }
        created_threads++;
      }

      // Print the progression table
      printf("%5s | %10s | %*s%s%*s |%*s%s%*s|\n", "Time", "Process ID", 3, "",
             "Status", 3, "", 1, "", "Remaining Time", 1, "");
      printf("------|------------|--------------|----------------|\n");

      // Main Loop
      while (completed_count < num_processes) {
        check_pthread(pthread_mutex_lock(&ctx->global_mutex),
      }

      // Thread function
      // DESIGN NOTE: Lock-Step Synchronization
      // This simulation uses a "lock-step" approach where the scheduler and
      // worker threads strictly alternate execution for each time unit (tick).
      // While this limits parallelism (only one thread runs at a time), it is
      // intentional to ensure deterministic simulation of the SRTF algorithm
      // and accurate time tracking. A true parallel execution would make it
      // difficult to guarantee exact 1-second ticks.
      void *process_thread_function(void *arg) {
        Process *p = (Process *)arg;
        SimulationContext *ctx = p->ctx;

        while (1) {
          // lock
          check_pthread(pthread_mutex_lock(&ctx->global_mutex),
                        "Lock mutex in thread");

          // until the process turn into runnable, wait
          while (p->run_permission == 0 && !ctx->simulation_finished) {
            check_pthread(pthread_cond_wait(&p->my_cond, &ctx->global_mutex),
                          "Wait cond in thread");
          }

          // if simulation is done
          if (ctx->simulation_finished) {
            check_pthread(pthread_mutex_unlock(&ctx->global_mutex),
                          "Unlock mutex in thread (finish)");
            break;
          }

          // Critical Section
          if (p->RemainingBurstTime > 0) {
            p->RemainingBurstTime--;
          }

          // notify to the scheduler
          p->run_permission = 0;
          ctx->tick_completed = 1;
          check_pthread(pthread_cond_signal(&ctx->scheduler_cond),
                        "Signal scheduler");

          // unlock
          check_pthread(pthread_mutex_unlock(&ctx->global_mutex),
                        "Unlock mutex in thread");

          // if done get out from thread loop
          if (p->RemainingBurstTime == 0) {
            break;
          }
        }
        return NULL;
      }

      // Helper to read integer input safely
      int read_int(const char *prompt, int min, int max) {
        int value;
        char buffer[100];

        while (1) {
          printf("%s", prompt);
          if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
            char *endptr;
            value = strtol(buffer, &endptr, 10);

            if (endptr == buffer || *endptr != '\n') {
              printf("Invalid input. Please enter a number.\n");
            } else if (value < min || value > max) {
              printf("Error: Number must be between %d and %d.\n", min, max);
            } else {
              return value;
            }
          }
        }
      }

      // 1. Receive the process info
      void initialize_processes(int num_processes, Process *processes,
                                SimulationContext *ctx) {
        for (int i = 0; i < num_processes; i++) {
          char prompt[100];

          printf("Process %d\n", i + 1);

          // Arrival Time
          snprintf(prompt, sizeof(prompt), "Arrival Time: ");
          int AT = read_int(prompt, 0, INT_MAX);

          // Burst Time
          snprintf(prompt, sizeof(prompt), "Burst Time: ");
          int BT = read_int(prompt, 1, INT_MAX);

          printf("\n");

          // initialize
          processes[i].ID = i + 1;
          processes[i].ArrivalTime = AT;
          processes[i].BurstTime = BT;
          processes[i].RemainingBurstTime = BT;
          processes[i].CompletionTime = 0;
          processes[i].ResponseTime = -1;
          processes[i].status = STATE_READY;

          // intialize variable for multithread
          processes[i].run_permission = 0;
          processes[i].ctx = ctx;

          // init directory the memory to initialize with the correct memory
          // address
          check_pthread(pthread_cond_init(&processes[i].my_cond, NULL),
                        "Init process cond");
        }
      }

      // 2. Need to find the Shortest Remaining Time (returns the index of the
      // process)
      int search_shortest(int num_processes, Process *processes,
                          int current_time) {
        int shortest_index =
            -1; // index of process which has the Shortest Remaining Time
        int min_remaining_time = INT_MAX; // minimum remaining time

        for (int i = 0; i < num_processes; i++) {
          // check whether the process is arrived by comparing with current time
          if (processes[i].ArrivalTime <= current_time) {
            // check whether the process is not completed
            if (processes[i].RemainingBurstTime > 0) {
              // find the shortest remaining time
              if (processes[i].RemainingBurstTime < min_remaining_time) {
                // Update the SRT
                min_remaining_time = processes[i].RemainingBurstTime;
                // Update the index
                shortest_index = i;
              }
            }
          }
        }
        return shortest_index;
      }

      /*
      |  P1  |  P2  |  P3  |...
      0      2      4      5...
      */
      void gantt_chart(SimulationContext * ctx) {
        printf("\n\n--- Visual Gantt Chart ---\n\n");

        if (ctx->log_index == 0)
          return;

        int current_pid = ctx->log_buffer[0].process_id;

        // print the 1st row
        for (int i = 1; i < ctx->log_index; i++) {
          if (ctx->log_buffer[i].status == STATE_COMPLETED)
            continue; // if COMPLETED, just skip

          // if the process ID has changed
          if (ctx->log_buffer[i].process_id != current_pid) {
            if (current_pid == -1) {
              printf("|  IDLE "); // 7 char
            } else {
              printf("|  P%-4d", current_pid); // 7char: "| P1   "
            }
            current_pid = ctx->log_buffer[i].process_id;
          }
        }

        // fin loop
        // print the last block
        if (current_pid == -1) {
          printf("|  IDLE |\n");
        } else {
          printf("|  P%-4d|\n", current_pid);
        }

        // print the time
        current_pid = ctx->log_buffer[0].process_id;
        printf("%-8d", 0); // print the initial time(0)

        for (int i = 1; i < ctx->log_index; i++) {
          if (ctx->log_buffer[i].status == STATE_COMPLETED)
            continue;

          if (ctx->log_buffer[i].process_id != current_pid) {
            // if the curernt process had changed
            // print the time for new process
            printf("%-8d", ctx->log_buffer[i].time); // 7char
            current_pid = ctx->log_buffer[i].process_id;
          }
        }

        // find the last completion time
        int final_time = 0;
        for (int i = ctx->log_index - 1; i >= 0; i--) {
          if (ctx->log_buffer[i].status == STATE_COMPLETED &&
              ctx->log_buffer[i].time > final_time) {
            final_time = ctx->log_buffer[i].time;
          }
        }
        if (final_time == 0 && ctx->log_index > 0)
          final_time = ctx->log_buffer[ctx->log_index - 1].time + 1;

        printf("%-7d\n", final_time);
      }

      // Helper to print a row in the progression table
      void print_row(int time, const char *pid_str, const char *status_str,
                     const char *rem_str) {
        int w_pid = COL_WIDTH_PID;
        int w_stat = COL_WIDTH_STAT;
        int w_rem = COL_WIDTH_REM;

        int l_pid = (w_pid - (int)strlen(pid_str)) / 2;
        int r_pid = w_pid - (int)strlen(pid_str) - l_pid;

        int l_stat = (w_stat - (int)strlen(status_str)) / 2;
        int r_stat = w_stat - (int)strlen(status_str) - l_stat;

        int l_rem = (w_rem - (int)strlen(rem_str)) / 2;
        int r_rem = w_rem - (int)strlen(rem_str) - l_rem;

        printf("%5d |%*s%s%*s|%*s%s%*s|%*s%s%*s|\n", time, l_pid, "", pid_str,
               r_pid, "", l_stat, "", status_str, r_stat, "", l_rem, "",
               rem_str, r_rem, "");
      }

      // Helper to log state
      void add_log(SimulationContext * ctx, int time, int pid, int remaining,
                   process_state_t status) {
        if (ctx->log_index >= MAX_LOG_SIZE) {
          fprintf(stderr, "Error: Log buffer overflow. Max size is %d.\n",
                  MAX_LOG_SIZE);
          exit(EXIT_FAILURE);
        }
        ctx->log_buffer[ctx->log_index++] =
            (GanttLog){time, pid, remaining, status};
      }

      // Print performance metrics
      void print_performance_metrics(int num_processes, Process *processes) {
        float Avg_TAT = 0.0;
        float Avg_WT = 0.0;
        float Avg_RT = 0.0;

        printf("\n\n--- SRTF Performance Results ---\n");
        for (int i = 0; i < num_processes; i++) {
          processes[i].TurnaroundTime =
              processes[i].CompletionTime - processes[i].ArrivalTime;
          Avg_TAT += processes[i].TurnaroundTime;

          processes[i].WaitingTime =
              processes[i].TurnaroundTime - processes[i].BurstTime;
          Avg_WT += processes[i].WaitingTime;

          int response_time =
              processes[i].ResponseTime - processes[i].ArrivalTime;
          Avg_RT += response_time;

          printf("Process %d: Turnaround = %d, Waiting = %d, Response = %d\n",
                 processes[i].ID, processes[i].TurnaroundTime,
                 processes[i].WaitingTime, response_time);
        }
        printf("\nAverage Turnaround Time = %.2f\n", Avg_TAT / num_processes);
        printf("Average Waiting Time = %.2f\n", Avg_WT / num_processes);
        printf("Average Response Time = %.2f\n", Avg_RT / num_processes);
      }

      // Run the scheduler loop
      void run_scheduler(int num_processes, Process *processes,
                         SimulationContext *ctx) {
        int current_time = 0;
        int completed_count = 0;
        int processing_process_index = -1;
        int created_threads = 0;

        // create thread
        for (int i = 0; i < num_processes; i++) {
          if (pthread_create(&processes[i].thread, NULL,
                             process_thread_function, &processes[i]) != 0) {
            fprintf(stderr, "Failed to create thread for process %d\n",
                    processes[i].ID);
            goto cleanup;
          }
          created_threads++;
        }

        // Print the progression table
        printf("%5s | %10s | %*s%s%*s |%*s%s%*s|\n", "Time", "Process ID", 3,
               "", "Status", 3, "", 1, "", "Remaining Time", 1, "");
        printf("------|------------|--------------|----------------|\n");

        // Main Loop
        while (completed_count < num_processes) {
          check_pthread(pthread_mutex_lock(&ctx->global_mutex),
                        "Lock mutex in scheduler");

          int shortest_index =
              search_shortest(num_processes, processes, current_time);

          if (shortest_index != -1) {
            if (processing_process_index != -1 &&
                processing_process_index != shortest_index) {
              if (processes[processing_process_index].status !=
                  STATE_COMPLETED) {
                processes[processing_process_index].status = STATE_READY;
              }
            }

            if (processes[shortest_index].ResponseTime == -1) {
              processes[shortest_index].ResponseTime = current_time;
              char pid_str[14], rem_str[18];
              snprintf(pid_str, sizeof(pid_str), "P%d",
                       processes[shortest_index].ID);
              snprintf(rem_str, sizeof(rem_str), "%d",
                       processes[shortest_index].RemainingBurstTime);
              print_row(current_time, pid_str, "READY", rem_str);
            }

            processes[shortest_index].status = STATE_RUNNING;
            add_log(ctx, current_time, processes[shortest_index].ID,
                    processes[shortest_index].RemainingBurstTime,
                    STATE_RUNNING);

            {
              char pid_str[12], rem_str[18];
              snprintf(pid_str, sizeof(pid_str), "P%d",
                       processes[shortest_index].ID);
              snprintf(rem_str, sizeof(rem_str), "%d",
                       processes[shortest_index].RemainingBurstTime);
              print_row(current_time, pid_str,
                        status_to_string(processes[shortest_index].status),
                        rem_str);
            }

            ctx->tick_completed = 0;
            processes[shortest_index].run_permission = 1;
            check_pthread(
                pthread_cond_signal(&processes[shortest_index].my_cond),
                "Signal process");

            while (ctx->tick_completed == 0) {
              check_pthread(
                  pthread_cond_wait(&ctx->scheduler_cond, &ctx->global_mutex),
                  "Wait for tick");
            }

            if (processes[shortest_index].RemainingBurstTime == 0) {
              processes[shortest_index].status = STATE_COMPLETED;
              completed_count++;
              processes[shortest_index].CompletionTime = current_time + 1;

              add_log(ctx, current_time + 1, processes[shortest_index].ID, 0,
                      STATE_COMPLETED);

              {
                char pid_str[12];
                snprintf(pid_str, sizeof(pid_str), "P%d",
                         processes[shortest_index].ID);
                print_row(current_time + 1, pid_str, "COMPLETED", "0");
              }
            }
            processing_process_index = shortest_index;
          } else {
            add_log(ctx, current_time, -1, 0, STATE_READY);
            print_row(current_time, "---", "IDLE", "---");
            processing_process_index = -1;
          }
          current_time++;
          check_pthread(pthread_mutex_unlock(&ctx->global_mutex),
                        "Unlock mutex in scheduler");
        }

        // Normal exit cleanup
        check_pthread(pthread_mutex_lock(&ctx->global_mutex),
                      "Lock mutex for finish");
        ctx->simulation_finished = 1;
        for (int i = 0; i < num_processes; i++) {
          check_pthread(pthread_cond_signal(&processes[i].my_cond),
                        "Signal finish");
        }
        check_pthread(pthread_mutex_unlock(&ctx->global_mutex),
                      "Unlock mutex for finish");

        for (int i = 0; i < num_processes; i++) {
          check_pthread(pthread_join(processes[i].thread, NULL), "Join thread");
        }
        return;

      cleanup:
        // Error cleanup
        // Signal any created threads to exit
        check_pthread(pthread_mutex_lock(&ctx->global_mutex),
                      "Lock mutex for cleanup");
        ctx->simulation_finished = 1;
        for (int i = 0; i < created_threads; i++) {
          check_pthread(pthread_cond_signal(&processes[i].my_cond),
                        "Signal cleanup");
        }
        check_pthread(pthread_mutex_unlock(&ctx->global_mutex),
                      "Unlock mutex for cleanup");

        for (int i = 0; i < created_threads; i++) {
          pthread_join(processes[i].thread, NULL);
        }
        exit(EXIT_FAILURE);
      }

      // Main
      int main() {
        // Initialize Context
        SimulationContext ctx;
        check_pthread(pthread_mutex_init(&ctx.global_mutex, NULL),
                      "Init global mutex");
        check_pthread(pthread_cond_init(&ctx.scheduler_cond, NULL),
                      "Init scheduler cond");
        ctx.tick_completed = 0;
        ctx.simulation_finished = 0;
        ctx.log_index = 0;

        int num_processes =
            read_int("Enter number of processes (1-10): ", 1, 10);

        // Dynamic allocation for processes
        Process *processes = calloc(num_processes, sizeof(Process));
        if (processes == NULL) {
          perror("Failed to allocate memory for processes");
          exit(EXIT_FAILURE);
        }

        initialize_processes(num_processes, processes, &ctx);

        run_scheduler(num_processes, processes, &ctx);

        gantt_chart(&ctx);
        print_performance_metrics(num_processes, processes);

        // Cleanup
        for (int i = 0; i < num_processes; i++) {
          check_pthread(pthread_cond_destroy(&processes[i].my_cond),
                        "Destroy process cond");
        }
        free(processes);
        check_pthread(pthread_mutex_destroy(&ctx.global_mutex),
                      "Destroy global mutex");
        check_pthread(pthread_cond_destroy(&ctx.scheduler_cond),
                      "Destroy scheduler cond");

        return 0;
      }