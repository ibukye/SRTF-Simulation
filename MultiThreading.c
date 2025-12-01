#include <ctype.h>
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
/*
Process 1: Arrival = 0, Burst = 7
Process 2: Arrival = 2, Burst = 4
Process 3: Arrival = 4, Burst = 1
Process 4: Arrival = 5, Burst = 4
*/
void initialize_process(int num_of_process, Process *All_Process) {
  for (int i = 0; i < num_of_process; i++) {
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
    All_Process[i].ID = i + 1;
    All_Process[i].ArrivalTime = AT;
    All_Process[i].BurstTime = BT;
    All_Process[i].RemainingBurstTime = BT;
    All_Process[i].CompletionTime = 0;
    All_Process[i].ResponseTime = -1;
    All_Process[i].status = READY;

    // intialize variable for multithread
    All_Process[i].run_permission = 0;

    // init directory the memory to initialize with the correct memory address
    pthread_cond_init(&All_Process[i].my_cond, NULL);
  }
  return;
}

// 2. Need to find the Shortest Remaining Time (returns the index of the
// process)
int search_shortest(int num_of_process, Process *All_Process,
                    int current_time) {
  int shortest_index =
      -1; // index of process which has the Shortest Remaining Time
  int min_remaining_time = INT_MAX; // minimum remaining time

  for (int i = 0; i < num_of_process; i++) {
    // check whether the process is arrived by comparing with current time
    if (All_Process[i].ArrivalTime <= current_time) {
      // check whether the process is not completed
      if (All_Process[i].RemainingBurstTime > 0) {
        // find the shortest remaining time
        if (All_Process[i].RemainingBurstTime < min_remaining_time) {
          // Update the SRT
          min_remaining_time = All_Process[i].RemainingBurstTime;
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
  int start_time = 0;

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

// Main
int main() {
  int current_time = 0;              // timer
  int completed_count = 0;           // Count for completed process
  int num_of_process;                // total number of processes
  int processing_process_index = -1; // current process

  // Get how many processes are there
  printf("Enter number of processes (1-10): ");
  // loop until get valid input
  while (1) {
    if (scanf("%d", &num_of_process) == 1) {
      if (num_of_process <= 0 || num_of_process > 10) {
        printf("Error: Number must be between 1 and 10. Enter again: ");
      } else {
        break;
      }
    } else {
      printf("Invalid input. Please enter a number (1-10): ");
      // clear the buffer
      while ((num_of_process = getchar()) != '\n' && num_of_process != EOF)
        ;
    }
  }

  // list for all processes needs to be process
  Process All_Process[num_of_process];

  // Initialize the all process
  initialize_process(num_of_process, All_Process);

  // create thread
  for (int i = 0; i < num_of_process; i++) {
    // create for each process
    if (pthread_create(&All_Process[i].thread, NULL, process_thread_function,
                       &All_Process[i]) != 0) {
      printf("Failed to create thread");
      return 1;
    }
  }

  // Print the progression table
  printf("%5s | %10s | %*s%s%*s |%*s%s%*s|\n", "Time", "Process ID", 3, "",
         "Status", 3, "", 1, "", "Remaining Time", 1, "");
  printf("------|------------|--------------|----------------|\n");

  // Main Loop
  while (completed_count < num_of_process) {
    // get the global lock
    pthread_mutex_lock(&global_mutex);

    // find shortest
    int shortest_index =
        search_shortest(num_of_process, All_Process, current_time);

    // Check if there is a runnable process
    if (shortest_index != -1) {

      // track for preemption (change process)
      if (processing_process_index != -1 &&
          processing_process_index != shortest_index) {
        if (All_Process[processing_process_index].status != COMPLETED) {
          All_Process[processing_process_index].status = READY;
        }
      }

      // init arrive (print READY)
      if (All_Process[shortest_index].ResponseTime == -1) {
        All_Process[shortest_index].ResponseTime = current_time;

        char pid_str[14], stat_str[16], rem_str[18];
        sprintf(pid_str, "P%d", All_Process[shortest_index].ID);
        strcpy(stat_str, "READY");
        sprintf(rem_str, "%d", All_Process[shortest_index].RemainingBurstTime);
        int w_pid = 12, w_stat = 14, w_rem = 16;

        // calc the padding
        int l_pid = (w_pid - (int)strlen(pid_str)) / 2;
        int r_pid = w_pid - (int)strlen(pid_str) - l_pid;

        int l_stat = (w_stat - (int)strlen(stat_str)) / 2;
        int r_stat = w_stat - (int)strlen(stat_str) - l_stat;

        int l_rem = (w_rem - (int)strlen(rem_str)) / 2;
        int r_rem = w_rem - (int)strlen(rem_str) - l_rem;

        printf("%5d |%*s%s%*s|%*s%s%*s|%*s%s%*s|\n", current_time, l_pid, "",
               pid_str, r_pid, "", l_stat, "", stat_str, r_stat, "", l_rem, "",
               rem_str, r_rem, "");
      }

      All_Process[shortest_index].status = RUNNING;

      Log[log_index++] =
          (GanttLog){current_time, All_Process[shortest_index].ID,
                     All_Process[shortest_index].RemainingBurstTime, RUNNING};

      // Print RUNNING log
      {
        char pid_str[12], stat_str[12], rem_str[18];
        sprintf(pid_str, "P%d", All_Process[shortest_index].ID);
        strcpy(stat_str, get_status(All_Process[shortest_index].status));
        sprintf(rem_str, "%d", All_Process[shortest_index].RemainingBurstTime);
        int w_pid = 12, w_stat = 14, w_rem = 16;
        int l_pid = (w_pid - (int)strlen(pid_str)) / 2;
        int r_pid = w_pid - (int)strlen(pid_str) - l_pid;
        int l_stat = (w_stat - (int)strlen(stat_str)) / 2;
        int r_stat = w_stat - (int)strlen(stat_str) - l_stat;
        int l_rem = (w_rem - (int)strlen(rem_str)) / 2;
        int r_rem = w_rem - (int)strlen(rem_str) - l_rem;

        printf("%5d |%*s%s%*s|%*s%s%*s|%*s%s%*s|\n", current_time, l_pid, "",
               pid_str, r_pid, "", l_stat, "", stat_str, r_stat, "", l_rem, "",
               rem_str, r_rem, "");
      }

      // MULTI THREAD
      // reset the flag
      tick_completed = 0;

      // permission
      All_Process[shortest_index].run_permission = 1;

      pthread_cond_signal(&All_Process[shortest_index].my_cond);

      // wait for 1s of process
      while (tick_completed == 0) {
        pthread_cond_wait(&scheduler_cond, &global_mutex);
      }

      // check if the process completed
      if (All_Process[shortest_index].RemainingBurstTime == 0) {
        All_Process[shortest_index].status = COMPLETED;
        completed_count++;
        All_Process[shortest_index].CompletionTime = current_time + 1;

        // record ot the log
        Log[log_index++] = (GanttLog){
            current_time + 1, All_Process[shortest_index].ID, 0, COMPLETED};

        {
          char pid_str[12], stat_str[12], rem_str[18];
          sprintf(pid_str, "P%d", All_Process[shortest_index].ID);
          strcpy(stat_str, "COMPLETED");
          sprintf(rem_str, "0");

          int w_pid = 12, w_stat = 14, w_rem = 16;
          int l_pid = (w_pid - (int)strlen(pid_str)) / 2;
          int r_pid = w_pid - (int)strlen(pid_str) - l_pid;
          int l_stat = (w_stat - (int)strlen(stat_str)) / 2;
          int r_stat = w_stat - (int)strlen(stat_str) - l_stat;
          int l_rem = (w_rem - (int)strlen(rem_str)) / 2;
          int r_rem = w_rem - (int)strlen(rem_str) - l_rem;

          // current_time + 1 since after the processing
          printf("%5d |%*s%s%*s|%*s%s%*s|%*s%s%*s|\n", current_time + 1, l_pid,
                 "", pid_str, r_pid, "", l_stat, "", stat_str, r_stat, "",
                 l_rem, "", rem_str, r_rem, "");
        }
      }

      processing_process_index = shortest_index;

    } else { // IDLE (searching for the next runnable processes)
      // CPU idle
      Log[log_index++] = (GanttLog){current_time, -1, 0, READY};

      char pid_str[12] = "---", stat_str[12] = "IDLE", rem_str[18] = "---";
      int w_pid = 12, w_stat = 14, w_rem = 16;

      int l_pid = (w_pid - (int)strlen(pid_str)) / 2;
      int r_pid = w_pid - (int)strlen(pid_str) - l_pid;
      int l_stat = (w_stat - (int)strlen(stat_str)) / 2;
      int r_stat = w_stat - (int)strlen(stat_str) - l_stat;
      int l_rem = (w_rem - (int)strlen(rem_str)) / 2;
      int r_rem = w_rem - (int)strlen(rem_str) - l_rem;

      printf("%5d |%*s%s%*s|%*s%s%*s|%*s%s%*s|\n", current_time, l_pid, "",
             pid_str, r_pid, "", l_stat, "", stat_str, r_stat, "", l_rem, "",
             rem_str, r_rem, "");

      processing_process_index = -1;
    }
    // Update time
    current_time++;
    // unlcok
    pthread_mutex_unlock(&global_mutex);
  }

  // simulation fin
  pthread_mutex_lock(&global_mutex);
  simulation_finished = 1;
  // fin all thread
  for (int i = 0; i < num_of_process; i++) {
    pthread_cond_signal(&All_Process[i].my_cond);
  }
  pthread_mutex_unlock(&global_mutex);

  // thread join
  for (int i = 0; i < num_of_process; i++) {
    pthread_join(All_Process[i].thread, NULL);
  }

  // result of computation and display
  //  Gantt Chart
  gantt_chart(Log, log_index);

  // SRTF PERFORMANCE RESULTS
  // for Average Turnaround Time, Waiting Time
  float Avg_TAT = 0.0;
  float Avg_WT = 0.0;
  float Avg_RT = 0.0;

  printf("\n\n--- SRTF Performance Results ---\n");
  for (int i = 0; i < num_of_process; i++) {

    // TAT = CT - AT
    All_Process[i].TurnaroundTime =
        All_Process[i].CompletionTime - All_Process[i].ArrivalTime;
    Avg_TAT += All_Process[i].TurnaroundTime;

    // WT = TAT - BT
    All_Process[i].WaitingTime =
        All_Process[i].TurnaroundTime - All_Process[i].BurstTime;
    Avg_WT += All_Process[i].WaitingTime;

    // Response Time (initial process - AT)
    int response_time = All_Process[i].ResponseTime -
                        All_Process[i].ArrivalTime; //  応答時間を計算
    Avg_RT += response_time;

    // PRINT
    printf("Process %d: Turnaround = %d, Waiting = %d, Response = %d\n",
           All_Process[i].ID, All_Process[i].TurnaroundTime,
           All_Process[i].WaitingTime, response_time);
  }
  printf("\nAverage Turnaround Time = %.2f\n", Avg_TAT / num_of_process);
  printf("Average Waiting Time = %.2f\n", Avg_WT / num_of_process);
  printf("Average Response Time = %.2f\n", Avg_RT / num_of_process);

  // release the resource
  for (int i = 0; i < num_of_process; i++) {
    pthread_cond_destroy(&All_Process[i].my_cond);
  }
  pthread_mutex_destroy(&global_mutex);
  pthread_cond_destroy(&scheduler_cond);

  return 0;
}