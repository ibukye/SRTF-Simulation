#include <stdio.h>

// state definition
#define READY 0 
#define RUNNING 1
#define COMPLETED 2

// process structure
typedef struct {
    int ID;         // process ID
    int ArrivalTime;
    int BurstTime;
    int RemainingBurstTime;
    int CompletionTime;
    // include?
    int status;
    int TrunAroundTime;
    int WaitingTime;
    int ResponseTime;
} Process;

// implementation & usage of ready queue might increase overhead


// 1. Receive the process info
/*
Process 1: Arrival = 0, Burst = 7
Process 2: Arrival = 2, Burst = 4
Process 3: Arrival = 4, Burst = 1
Process 4: Arrival = 5, Burst = 4
*/
void initialize_process(int num_of_process, Process *All_Process) {
    for (int i = 0; i < num_of_process; i++) {
        Process process;
        int AT, BT;         // ArrivalTime, BurstTime

        // get user input
        printf("Process %d: Arrival = ", i+1);
        scanf("%d", &AT);
        printf(", Burst = ");
        scanf("%d", &BT);
        printf("\n");

        // initialize
        process.ID = i+1;
        process.ArrivalTime = AT;
        process.BurstTime = BT;
        process.RemainingBurstTime = BT;
        process.CompletionTime = 0;
        process.ResponseTime = 10000;
        
        All_Process[i] = process;
    }
    return;
}

// 2. Need to find the Shortest Remaining Time (returns the index of the process)
int search_shortest(int num_of_process, Process *All_Process, int current_time) {
    int shortest_index = -1;                    // index of process which has the Shortest Remaining Time
    int min_remaining_time = 10000000000;       // minimum remaining time

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

// 3. Process the SRT process and set the RT for the process (loop every unit seconds)
/*void Processing(int num_of_process, Process *All_Process, int current_time) {

}*/


// get status char
char* get_status(int status) {
    switch (status) {
        case READY: return "READY";
        case RUNNING: return "RUNNING";
        case COMPLETED: return "COMPLETED";
        default: return "NONE";
    }
}










// Main
int main() {
    int current_time = 0;               // timer
    int completed_count = 0;            // Count for completed process
    int num_of_process;                 // total number of processes
    //int processing_process_index = -1;  // current process

    // 1. Get how many processes are there
    printf("Enter number of processes (1-10): ");
    scanf("%d", &num_of_process);
    
    // list for all processes needs to be process
    Process All_Process[num_of_process];

    // 2. Initialize the all process
    initialize_process(num_of_process, All_Process);

    // Print the progression table 
    printf("%5s | %10s | %12s | %15s|\n", "Time", "Process ID", "Status", "Remaining Time");

    // 3. Main Loop
    while (completed_count < num_of_process) {
        // find shortest
        int shortest_index = search_shortest(num_of_process, All_Process, current_time);
        // update the current process

        // RUN (decrement by one unit time)
        // if current time is smaller than the initial response time
        if (current_time < All_Process[shortest_index].ResponseTime ) {
            All_Process[shortest_index].ResponseTime = current_time;
        }
        All_Process[shortest_index].RemainingBurstTime--;
        All_Process[shortest_index].status = RUNNING;

        // check whehter its done
        if (All_Process[shortest_index].RemainingBurstTime == 0) {
            All_Process[shortest_index].status = COMPLETED;
            completed_count++;
            All_Process[shortest_index].CompletionTime = current_time+1;    // record the completion time after incremented
        }
        
        // Update time
        current_time++;

        // REVIEW & TABLE OF THE PROCESSING (?)
        Process p = All_Process[shortest_index];
        // get char from status
        printf("%5d | P%10d | %12s | %15d |\n", current_time, p.ID, get_status(p.status), p.RemainingBurstTime);
    }

    // SRTF PERFORMANCE RESULTS
    // for Average Turnaround Time, Waiting Time
    float Avg_TAT = 0.0;
    float Avg_WT = 0.0;
    for (int i = 0; i < num_of_process; i++) {
        Process p = All_Process[i];
        
        // TAT = CT - AT
        p.TrunAroundTime = p.CompletionTime - p.ArrivalTime;
        Avg_TAT += p.TrunAroundTime;
        
        // WT = TAT - BT
        p.WaitingTime = p.TrunAroundTime - p.BurstTime;
        Avg_WT += p.WaitingTime;
        
        // Response Time (initial process - AT)

        // PRINT
        printf("Process %d: Turnaround = %d, Waiting = %d, Response = %d\n", p.ID, p.TrunAroundTime, p.WaitingTime, p.ResponseTime-p.ArrivalTime);
    }
    printf("\nAverage Turnaround Time = %.2f\nAverage Waiting Time = %.2f", Avg_TAT/num_of_process, Avg_WT/num_of_process);
}
