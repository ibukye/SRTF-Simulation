#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>


// state definition
#define READY 0
#define RUNNING 1
#define COMPLETED 2

#define MAX_LOG_SIZE 1000 // max running time for log

// Multi Threading Global Variable
pthread_mutex_t global_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t scheduler_cond = PTHREAD_COND_INITIALIZER;
int tick_completed = 0; // flag to check whether the process done 1s of process
int simulation_finished = 0; // flag to indicate fin of simulation

// process structure
typedef struct {
  int ID; // process ID
  int ArrivalTime;
  int BurstTime;
  int RemainingBurstTime;
  int CompletionTime;
  int status;
  int TurnaroundTime;
  int WaitingTime;
  int ResponseTime;

  // For multi threading
  pthread_t thread;       // thread body
  pthread_cond_t my_cond; //
  int run_permission;     // flag to indicate whether this process is runnable
} Process;

// struct for recording logs
typedef struct {
  int time;
  int process_id; // -1 for idle
  int remaining_time;
  int status;
} GanttLog;

GanttLog Log[MAX_LOG_SIZE];
int log_index = 0; // index of current log

// Thread function
void *process_thread_function(void *arg) {
  Process *p = (Process *)arg;

  while (1) {
    // lock
    pthread_mutex_lock(&global_mutex);

    // until the process turn into runnable, wait
    while (p->run_permission == 0 && !simulation_finished) {
      pthread_cond_wait(&p->my_cond, &global_mutex);
    }

    // if simulation is done
    if (simulation_finished) {
      pthread_mutex_unlock(&global_mutex);
      break;
    }

    // Critical Section
    if (p->RemainingBurstTime > 0) {
      p->RemainingBurstTime--;
    }

    // notify to the scheduler
    p->run_permission = 0;
    tick_completed = 1;
    pthread_cond_signal(&scheduler_cond);

    // unlock
    pthread_mutex_unlock(&global_mutex);

    // if done get out from thread loop
    if (p->RemainingBurstTime == 0) {
      break;
    }
  }
  return NULL;
}

// 1. Receive the process info
void initialize_processes(int num_processes, Process *processes) {
  for (int i = 0; i < num_processes; i++) {
    int AT, BT; // ArrivalTime, BurstTime

    // get user input
    printf("Process %d\nArrival (non-negative number): ", i + 1);
    while (1) {
      if (scanf("%d", &AT) == 1) {
        if (AT >= 0) {
          break;
        } else {
          printf("Error: Arrival time cannot be negative. Enter again: ");
        }
      } else {
        printf("Invalid input. Please enter a number: ");
        while ((AT = getchar()) != '\n' && AT != EOF)
          ;
      }
    }
    printf("Burst (non-negative number): ");
    while (1) {
      if (scanf("%d", &BT) == 1) {
        if (BT > 0) {
          break;
        } else {
          printf("Error: Burst time must be greater than 0. Enter again: ");
        }
      } else {
        printf("Invalid input. Please enter a number: ");
        while ((BT = getchar()) != '\n' && BT != EOF)
          ;
      }
    }
    printf("\n");

    // initialize
    processes[i].ID = i + 1;
    processes[i].ArrivalTime = AT;
    processes[i].BurstTime = BT;
    processes[i].RemainingBurstTime = BT;
    processes[i].CompletionTime = 0;
    processes[i].ResponseTime = -1;
    processes[i].status = READY;

    // intialize variable for multithread
    processes[i].run_permission = 0;

    // init directory the memory to initialize with the correct memory address
    pthread_cond_init(&processes[i].my_cond, NULL);
  }
  return;
}

// 2. Need to find the Shortest Remaining Time (returns the index of the
// process)
int search_shortest(int num_processes, Process *processes, int current_time) {
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

// get status char
char *get_status(int status) {
  switch (status) {
  case READY:
    return "READY";
  case RUNNING:
    return "RUNNING";
  case COMPLETED:
    return "COMPLETED";
  default:
    return "NONE";
  }
}

/*
|  P1  |  P2  |  P3  |...
0      2      4      5...
*/
void gantt_chart(GanttLog *Log, int log_index) {
  printf("\n\n--- Visual Gantt Chart ---\n\n");

  if (log_index == 0)
    return;

  int current_pid = Log[0].process_id;

  // print the 1st row
  for (int i = 1; i < log_index; i++) {
    if (Log[i].status == COMPLETED)
      continue; // if COMPLETED, just skip

    // if the process ID has changed
    if (Log[i].process_id != current_pid) {
      if (current_pid == -1) {
        printf("|  IDLE "); // 7 char
      } else {
        printf("|  P%-4d", current_pid); // 7char: "| P1   "
      }
      current_pid = Log[i].process_id;
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
  current_pid = Log[0].process_id;
  printf("%-8d", 0); // print the initial time(0)

  for (int i = 1; i < log_index; i++) {
    if (Log[i].status == COMPLETED)
      continue;

    if (Log[i].process_id != current_pid) {
      // if the curernt process had changed
      // print the time for new process
      printf("%-8d", Log[i].time); // 7char
      current_pid = Log[i].process_id;
    }
  }

  // find the last completion time
  int final_time = 0;
  for (int i = log_index - 1; i >= 0; i--) {
    if (Log[i].status == COMPLETED && Log[i].time > final_time) {
      final_time = Log[i].time;
    }
  }
  if (final_time == 0 && log_index > 0)
    final_time = Log[log_index - 1].time + 1;

  printf("%-7d\n", final_time);
}

// Helper to print a row in the progression table
void print_row(int time, const char *pid_str, const char *status_str,
               const char *rem_str) {
  int w_pid = 12, w_stat = 14, w_rem = 16;

  int l_pid = (w_pid - (int)strlen(pid_str)) / 2;
  int r_pid = w_pid - (int)strlen(pid_str) - l_pid;

  int l_stat = (w_stat - (int)strlen(status_str)) / 2;
  int r_stat = w_stat - (int)strlen(status_str) - l_stat;

  int l_rem = (w_rem - (int)strlen(rem_str)) / 2;
  int r_rem = w_rem - (int)strlen(rem_str) - l_rem;

  printf("%5d |%*s%s%*s|%*s%s%*s|%*s%s%*s|\n", time, l_pid, "", pid_str, r_pid,
         "", l_stat, "", status_str, r_stat, "", l_rem, "", rem_str, r_rem, "");
}

// Run the scheduler loop
void run_scheduler(int num_processes, Process *processes) {
  int current_time = 0;
  int completed_count = 0;
  int processing_process_index = -1;

  // create thread
  for (int i = 0; i < num_processes; i++) {
    if (pthread_create(&processes[i].thread, NULL, process_thread_function,
                       &processes[i]) != 0) {
      printf("Failed to create thread");
      return;
    }
  }

  // Print the progression table
  printf("%5s | %10s | %*s%s%*s |%*s%s%*s|\n", "Time", "Process ID", 3, "",
         "Status", 3, "", 1, "", "Remaining Time", 1, "");
  printf("------|------------|--------------|----------------|\n");

  // Main Loop
  while (completed_count < num_processes) {
    pthread_mutex_lock(&global_mutex);

    int shortest_index =
        search_shortest(num_processes, processes, current_time);

    if (shortest_index != -1) {
      if (processing_process_index != -1 &&
          processing_process_index != shortest_index) {
        if (processes[processing_process_index].status != COMPLETED) {
          processes[processing_process_index].status = READY;
        }
      }

      if (processes[shortest_index].ResponseTime == -1) {
        processes[shortest_index].ResponseTime = current_time;
        char pid_str[14], rem_str[18];
        sprintf(pid_str, "P%d", processes[shortest_index].ID);
        sprintf(rem_str, "%d", processes[shortest_index].RemainingBurstTime);
        print_row(current_time, pid_str, "READY", rem_str);
      }

      processes[shortest_index].status = RUNNING;
      Log[log_index++] =
          (GanttLog){current_time, processes[shortest_index].ID,
                     processes[shortest_index].RemainingBurstTime, RUNNING};

      {
        char pid_str[12], rem_str[18];
        sprintf(pid_str, "P%d", processes[shortest_index].ID);
        sprintf(rem_str, "%d", processes[shortest_index].RemainingBurstTime);
        print_row(current_time, pid_str,
                  get_status(processes[shortest_index].status), rem_str);
      }

      tick_completed = 0;
      processes[shortest_index].run_permission = 1;
      pthread_cond_signal(&processes[shortest_index].my_cond);

      while (tick_completed == 0) {
        pthread_cond_wait(&scheduler_cond, &global_mutex);
      }

      if (processes[shortest_index].RemainingBurstTime == 0) {
        processes[shortest_index].status = COMPLETED;
        completed_count++;
        processes[shortest_index].CompletionTime = current_time + 1;

        Log[log_index++] = (GanttLog){
            current_time + 1, processes[shortest_index].ID, 0, COMPLETED};

        {
          char pid_str[12];
          sprintf(pid_str, "P%d", processes[shortest_index].ID);
          print_row(current_time + 1, pid_str, "COMPLETED", "0");
        }
      }
      processing_process_index = shortest_index;
    } else {
      Log[log_index++] = (GanttLog){current_time, -1, 0, READY};
      print_row(current_time, "---", "IDLE", "---");
      processing_process_index = -1;
    }
    current_time++;
    pthread_mutex_unlock(&global_mutex);
  }

  pthread_mutex_lock(&global_mutex);
  simulation_finished = 1;
  for (int i = 0; i < num_processes; i++) {
    pthread_cond_signal(&processes[i].my_cond);
  }
  pthread_mutex_unlock(&global_mutex);

  for (int i = 0; i < num_processes; i++) {
    pthread_join(processes[i].thread, NULL);
  }
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

    int response_time = processes[i].ResponseTime - processes[i].ArrivalTime;
    Avg_RT += response_time;

    printf("Process %d: Turnaround = %d, Waiting = %d, Response = %d\n",
           processes[i].ID, processes[i].TurnaroundTime,
           processes[i].WaitingTime, response_time);
  }
  printf("\nAverage Turnaround Time = %.2f\n", Avg_TAT / num_processes);
  printf("Average Waiting Time = %.2f\n", Avg_WT / num_processes);
  printf("Average Response Time = %.2f\n", Avg_RT / num_processes);
}

// Main
int main() {
  int num_processes;

  // Get how many processes are there
  printf("Enter number of processes (1-10): ");
  while (1) {
    if (scanf("%d", &num_processes) == 1) {
      if (num_processes <= 0 || num_processes > 10) {
        printf("Error: Number must be between 1 and 10. Enter again: ");
      } else {
        break;
      }
    } else {
      printf("Invalid input. Please enter a number (1-10): ");
      while ((num_processes = getchar()) != '\n' && num_processes != EOF)
        ;
    }
  }

  Process processes[num_processes];
  initialize_processes(num_processes, processes);

  run_scheduler(num_processes, processes);

  gantt_chart(Log, log_index);

  print_performance_metrics(num_processes, processes);

  for (int i = 0; i < num_processes; i++) {
    pthread_cond_destroy(&processes[i].my_cond);
  }
  pthread_mutex_destroy(&global_mutex);
  pthread_cond_destroy(&scheduler_cond);

  return 0;
}