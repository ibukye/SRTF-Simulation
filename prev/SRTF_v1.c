#include <stdio.h>
#include <string.h>
#include <ctype.h>

// state definition
#define READY 0 
#define RUNNING 1
#define COMPLETED 2

#define MAX_LOG_SIZE 1000   // max running time for log

// process structure
typedef struct {
    int ID;         // process ID
    int ArrivalTime;
    int BurstTime;
    int RemainingBurstTime;
    int CompletionTime;
    // include?
    int status;
    int TurnaroundTime;
    int WaitingTime;
    int ResponseTime;
    // For multi threading
    int thread_id;

} Process;

// struct for recording logs
typedef struct {
    int time;
    int process_id;     // -1 for idle
    int remaining_time;
    int status;
} GanttLog;

GanttLog Log[MAX_LOG_SIZE];
int log_index = 0;      // index of current log

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
        // --- Arrival Time (AT) の検証 ---
        printf("Process %d: Arrival = ", i + 1);
        while (1) {
            if (scanf("%d", &AT) == 1) {
                if (AT >= 0) { // ATは0以上
                    break; 
                } else {
                    printf("Error: Arrival time cannot be negative. Enter again: ");
                }
            } else {
                printf("Invalid input. Please enter a number: ");
                while ((AT = getchar()) != '\n' && AT != EOF);
            }
        }

        // --- Burst Time (BT) の検証 ---
        printf("Burst = "); 
        while (1) {
            if (scanf("%d", &BT) == 1) {
                if (BT > 0) { // BTは0より大きい
                    break;
                } else {
                    printf("Error: Burst time must be greater than 0. Enter again: ");
                }
            } else {
                printf("Invalid input. Please enter a number: ");
                while ((BT = getchar()) != '\n' && BT != EOF);
            }
        }
        printf("\n");

        // initialize
        process.ID = i+1;
        process.ArrivalTime = AT;
        process.BurstTime = BT;
        process.RemainingBurstTime = BT;
        process.CompletionTime = 0;
        process.ResponseTime = 10000;
        process.status = READY;
        
        All_Process[i] = process;
    }
    return;
}

// 2. Need to find the Shortest Remaining Time (returns the index of the process)
int search_shortest(int num_of_process, Process *All_Process, int current_time) {
    int shortest_index = -1;                    // index of process which has the Shortest Remaining Time
    int min_remaining_time = 999999;       // minimum remaining time

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



//void calculate_print(int num_of_process, Process All_Process);
void gantt_chart(GanttLog *Log, int log_index) {
    printf("\n\n--- Visual Gantt Chart ---\n\n");
    
    if (log_index == 0) return;

    int current_pid = Log[0].process_id;
    int start_time = 0;
    
    // 1. トップ行（プロセスID）を先にすべて出力
    for (int i = 1; i < log_index; i++) {
        if (Log[i].status == COMPLETED) continue; // 完了ログはスキップ

        // プロセスIDが「変化」したかチェック
        if (Log[i].process_id != current_pid) {
            
            // --- 変化点 ---
            // 1. 直前のブロックを top_row に追加
            if (current_pid == -1) {
                printf("|  IDLE "); // 7文字幅
            } else {
                printf("|  P%-4d", current_pid); // 7文字幅: "| P1   "
            }
            current_pid = Log[i].process_id;
        }
    }
    // --- ループ終了後 ---
    // 最後のブロックを出力
    if (current_pid == -1) {
        printf("|  IDLE |\n");
    } else {
        printf("|  P%-4d|\n", current_pid);
    }

    // 2. ボトム行（時間）をすべて出力
    current_pid = Log[0].process_id;
    printf("%-8d", 0); // 最初の時間 0 を出力 (7文字幅)

    for (int i = 1; i < log_index; i++) {
        if (Log[i].status == COMPLETED) continue;

        if (Log[i].process_id != current_pid) {
            // --- 変化点 ---
            // 新しいブロックの開始時刻を出力
            printf("%-8d", Log[i].time); // 7文字幅
            current_pid = Log[i].process_id;
        }
    }
    
    // 最後の完了時刻を見つける
    int final_time = 0;
    for (int i = log_index - 1; i >= 0; i--) {
        if (Log[i].status == COMPLETED && Log[i].time > final_time) {
             final_time = Log[i].time;
        }
    }
    if (final_time == 0 && log_index > 0) final_time = Log[log_index - 1].time + 1;
    
    printf("%-7d\n", final_time);
}







// Main
int main() {
    int current_time = 0;               // timer
    int completed_count = 0;            // Count for completed process
    int num_of_process;                 // total number of processes
    int processing_process_index = -1;  // current process

    // 1. Get how many processes are there
    printf("Enter number of processes (1-10): ");
    // 無限ループで、正しい入力があるまで繰り返す
    while (1) {
        // scanfの戻り値 (1 == 成功) をチェック
        if (scanf("%d", &num_of_process) == 1) {
            
            // 成功した場合、意味的な検証 (1～10)
            // 🔴 0 を許可しないように <= 0 に変更
            if (num_of_process <= 0 || num_of_process > 10) {
                printf("Error: Number must be between 1 and 10. Enter again: ");
            } else {
                break; // 正常な入力。ループを抜ける
            }
        } else {
            // 失敗した場合 (文字が入力されたなど)
            printf("Invalid input. Please enter a number (1-10): ");
            // 🔴 バッファをクリア (改行まで読み飛ばす)
            while ((num_of_process = getchar()) != '\n' && num_of_process != EOF);
        }
    }
    
    // list for all processes needs to be process
    Process All_Process[num_of_process];

    // 2. Initialize the all process
    initialize_process(num_of_process, All_Process);

    // Print the progression table 
    printf("%5s | %10s | %*s%s%*s |%*s%s%*s|\n", "Time", "Process ID", 3, "", "Status", 3, "", 1, "", "Remaining Time", 1, "");
    printf("------|------------|--------------|----------------|\n");

    // 3. Main Loop
    // 3. Main Loop
    while (completed_count < num_of_process) {
        // find shortest
        int shortest_index = search_shortest(num_of_process, All_Process, current_time);

        // Check if there is a runnable process
        if (shortest_index != -1) {

            // 1. プリエンプションの追跡 (プロセス切り替え時の処理)
            if (processing_process_index != -1 && processing_process_index != shortest_index) {
                 if (All_Process[processing_process_index].status != COMPLETED) {
                    All_Process[processing_process_index].status = READY;
                }
            }

            // 2. 初回実行時 (到着時) の READY ログ出力
            if (All_Process[shortest_index].ResponseTime == 10000) {
                All_Process[shortest_index].ResponseTime = current_time;
                
                // --- 表示用変数の準備 (READY) ---
                char pid_str[14], stat_str[16], rem_str[18];
                sprintf(pid_str, "P%d", All_Process[shortest_index].ID);
                strcpy(stat_str, "READY"); // 🔴 強制的に READY と表示
                sprintf(rem_str, "%d", All_Process[shortest_index].RemainingBurstTime);

                // --- 幅の定義 (ヘッダーと一致させる) ---
                int w_pid = 12, w_stat = 14, w_rem = 16;
                
                // --- パディング計算 ---
                int l_pid = (w_pid - (int)strlen(pid_str)) / 2;
                int r_pid = w_pid - (int)strlen(pid_str) - l_pid;
                
                int l_stat = (w_stat - (int)strlen(stat_str)) / 2;
                int r_stat = w_stat - (int)strlen(stat_str) - l_stat;
                
                int l_rem = (w_rem - (int)strlen(rem_str)) / 2;
                int r_rem = w_rem - (int)strlen(rem_str) - l_rem;

                // --- READY 状態の出力 ---
                printf("%5d |%*s%s%*s|%*s%s%*s|%*s%s%*s|\n", 
                    current_time, 
                    l_pid, "", pid_str, r_pid, "", 
                    l_stat, "", stat_str, r_stat, "",
                    l_rem, "", rem_str, r_rem, "");
            }
            
            // 3. ステータスを RUNNING に更新
            All_Process[shortest_index].status = RUNNING;

            // 4. ログ記録 (RUNNING)
            Log[log_index++] = (GanttLog) {
                current_time,
                All_Process[shortest_index].ID,
                All_Process[shortest_index].RemainingBurstTime,
                RUNNING
            };

            // 5. 画面出力 (RUNNING)
            // 🔴 修正: 実行(減算)する「前」に表示することで、数値の飛び (7->6) を防ぐ
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

                printf("%5d |%*s%s%*s|%*s%s%*s|%*s%s%*s|\n", 
                    current_time, 
                    l_pid, "", pid_str, r_pid, "", 
                    l_stat, "", stat_str, r_stat, "",
                    l_rem, "", rem_str, r_rem, "");
            }

            // 6. 実行 (時間を減らす)
            All_Process[shortest_index].RemainingBurstTime--;
            
            // 7. 完了チェック
            if (All_Process[shortest_index].RemainingBurstTime == 0) {
                All_Process[shortest_index].status = COMPLETED;
                completed_count++;
                All_Process[shortest_index].CompletionTime = current_time + 1;
                
                // 完了ログ記録
                Log[log_index++] = (GanttLog) {
                    current_time + 1,
                    All_Process[shortest_index].ID,
                    0,
                    COMPLETED
                };

                // 🔴 オプション: 完了した瞬間も表示したい場合はここで printf (COMPLETED) を行う
                // (多くのGanttチャートでは、完了状態は次のタイムユニットの開始として扱われるか、サマリーで確認します)
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

                     // 完了した行を表示（好みによる。Timeは現在のままか、+1するか）
                     // ここではシンプルに「その秒の終わりに完了した」として表示
                     printf("%5d |%*s%s%*s|%*s%s%*s|%*s%s%*s|\n", 
                        current_time+1,  // 完了時刻を表示
                        l_pid, "", pid_str, r_pid, "", 
                        l_stat, "", stat_str, r_stat, "",
                        l_rem, "", rem_str, r_rem, "");
                }
            }

            processing_process_index = shortest_index;

        } else {
            // CPU idle
            Log[log_index++] = (GanttLog){current_time, -1, 0, READY};
            
            // --- IDLE 表示 ---
            char pid_str[12] = "---", stat_str[12] = "IDLE", rem_str[18] = "---";
            int w_pid = 12, w_stat = 14, w_rem = 16;

            int l_pid = (w_pid - (int)strlen(pid_str)) / 2;
            int r_pid = w_pid - (int)strlen(pid_str) - l_pid;
            int l_stat = (w_stat - (int)strlen(stat_str)) / 2;
            int r_stat = w_stat - (int)strlen(stat_str) - l_stat;
            int l_rem = (w_rem - (int)strlen(rem_str)) / 2;
            int r_rem = w_rem - (int)strlen(rem_str) - l_rem;

            printf("%5d |%*s%s%*s|%*s%s%*s|%*s%s%*s|\n", 
                current_time,
                l_pid, "", pid_str, r_pid, "",
                l_stat, "", stat_str, r_stat, "",
                l_rem, "", rem_str, r_rem, "");

            processing_process_index = -1;
        }

        // Update time
        current_time++;
    }

    //result of computation and display
    //calculate_print(num_of_process, All_Process);

    // Gantt Chart
    gantt_chart(Log, log_index);

    // SRTF PERFORMANCE RESULTS
    // for Average Turnaround Time, Waiting Time
    float Avg_TAT = 0.0;
    float Avg_WT = 0.0;
    float Avg_RT = 0.0;

    printf("\n\n--- SRTF Performance Results ---\n");
    for (int i = 0; i < num_of_process; i++) {

        // TAT = CT - AT
        All_Process[i].TurnaroundTime = All_Process[i].CompletionTime - All_Process[i].ArrivalTime;
        Avg_TAT += All_Process[i].TurnaroundTime;
        
        // WT = TAT - BT
        All_Process[i].WaitingTime = All_Process[i].TurnaroundTime - All_Process[i].BurstTime;
        Avg_WT += All_Process[i].WaitingTime;
        
        // Response Time (initial process - AT)
        int response_time = All_Process[i].ResponseTime - All_Process[i].ArrivalTime; // 🔴 応答時間を計算
        Avg_RT += response_time;

        // PRINT
        printf("Process %d: Turnaround = %d, Waiting = %d, Response = %d\n", All_Process[i].ID, All_Process[i].TurnaroundTime, All_Process[i].WaitingTime, response_time);
    }
    printf("\nAverage Turnaround Time = %.2f\n", Avg_TAT / num_of_process);
    printf("Average Waiting Time = %.2f\n", Avg_WT / num_of_process);
    printf("Average Response Time = %.2f\n", Avg_RT / num_of_process);

    return 0;
}