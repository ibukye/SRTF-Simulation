# MultiThreading.c 詳解（コード抜粋付き）

## 背景と目的
`MultiThreading.c` は、Shortest Remaining Time First（SRTF）プリエンプティブスケジューリングを POSIX スレッドで模擬するサンプルである。メインスレッドが「スケジューラ」として振る舞い、個々のプロセスは専用スレッドで 1 ティックずつ前進する。

```c
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <ctype.h>

#define READY 0
#define RUNNING 1
#define COMPLETED 2
#define MAX_LOG_SIZE 1000
```

---

## 主要データ構造と同期変数

### 状態・ログ関連の定義
- 状態定数 `READY / RUNNING / COMPLETED`
- 実行履歴を保持する `GanttLog`

```c
typedef struct {
    int time;
    int process_id;     // -1 for idle
    int remaining_time;
    int status;
} GanttLog;

GanttLog Log[MAX_LOG_SIZE];
int log_index = 0;
```

`process_id = -1` で CPU アイドルを記録するメタデータを実装している。

### Process 構造体

```c
typedef struct {
    int ID;
    int ArrivalTime;
    int BurstTime;
    int RemainingBurstTime;
    int CompletionTime;
    int status;
    int TurnaroundTime;
    int WaitingTime;
    int ResponseTime;

    pthread_t thread;
    pthread_cond_t my_cond;
    int run_permission;
} Process;
```

- `RemainingBurstTime` が SRTF の比較対象。
- `ResponseTime` は初回 RUNNING の時刻を記録するため初期値 10000 を代入して「未設定」を示す。
- 各プロセスが自分専用の `pthread_cond_t my_cond` を持ち、スケジューラが「実行許可」を出すときだけシグナルされる。

### グローバル同期オブジェクト

```c
pthread_mutex_t global_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t scheduler_cond = PTHREAD_COND_INITIALIZER;
int tick_completed = 0;
int simulation_finished = 0;
```

- `global_mutex`: スケジューラ・各プロセスが共有する排他制御。状態の変更や RUNNING 権限の切り替えはこのロック下で行われる。
- `scheduler_cond`: プロセスが 1 ティック分の実行を完了したことをメインループへ知らせる。
- `simulation_finished`: スレッド終了時の脱出条件。待機中のスレッドが永久待機に陥らないようにする。

---

## プログラムの流れ（該当コード抜粋付き）

### 1. 入力と初期化
最初にプロセス数を 1〜10 の範囲で受け取り、`initialize_process` で個々の `Process` を構築する。入力バリデーションでは `scanf` の戻り値を確認し、不正入力時はバッファをクリアして再入力を求めている。

```c
printf("Enter number of processes (1-10): ");
while (1) {
    if (scanf("%d", &num_of_process) == 1) {
        if (num_of_process <= 0 || num_of_process > 10) {
            printf("Error: Number must be between 1 and 10. Enter again: ");
        } else {
            break;
        }
    } else {
        printf("Invalid input. Please enter a number (1-10): ");
        while ((num_of_process = getchar()) != '\n' && num_of_process != EOF);
    }
}
```

```c
void initialize_process(int num_of_process, Process *All_Process) {
    for (int i = 0; i < num_of_process; i++) {
        int AT, BT;
        /* 省略: 入力バリデーション */
        All_Process[i].ID = i + 1;
        All_Process[i].ArrivalTime = AT;
        All_Process[i].BurstTime = BT;
        All_Process[i].RemainingBurstTime = BT;
        All_Process[i].CompletionTime = 0;
        All_Process[i].ResponseTime = 10000;
        All_Process[i].status = READY;
        All_Process[i].run_permission = 0;
        pthread_cond_init(&All_Process[i].my_cond, NULL);
    }
}
```

### 2. プロセススレッド生成
入力が終わると各プロセスをスレッド化する。スレッドは `process_thread_function` を実行し、スケジューラからの実行許可を待つワーカーとして機能する。

```c
for (int i = 0; i < num_of_process; i++) {
    if (pthread_create(&All_Process[i].thread, NULL,
                       process_thread_function, &All_Process[i]) != 0) {
        printf("Failed to create thread");
        return 1;
    }
}
```

### 3. プロセススレッド側の動作
`process_thread_function` は 1 ティックの実行→通知→再待機という流れを繰り返す。`run_permission` が 0 の間は条件変数 `my_cond` で待機し、スケジューラが `run_permission = 1` にセットしたときだけ起こされる。

```c
void* process_thread_function(void* arg) {
    Process* p = (Process*) arg;
    while (1) {
        pthread_mutex_lock(&global_mutex);
        while (p->run_permission == 0 && !simulation_finished) {
            pthread_cond_wait(&p->my_cond, &global_mutex);
        }
        if (simulation_finished) {
            pthread_mutex_unlock(&global_mutex);
            break;
        }
        if (p->RemainingBurstTime > 0) {
            p->RemainingBurstTime--;
        }
        p->run_permission = 0;
        tick_completed = 1;
        pthread_cond_signal(&scheduler_cond);
        pthread_mutex_unlock(&global_mutex);
        if (p->RemainingBurstTime == 0) {
            break;
        }
    }
    return NULL;
}
```

### 4. スケジューラ（メインループ）
メインスレッドは次の手順で SRTF を実現する。

1. `global_mutex` をロック。
2. `search_shortest` で現在時刻までに到着済みかつ未完了のうち最も残り時間の短いプロセスを探索。
3. 対象があれば RUNNING に設定し、無ければ IDLE として記録。
4. RUNNING のときは `run_permission = 1` にしシグナル、1 ティック完了通知を待機。
5. 残り時間が 0 になれば完了処理を実行。

```c
int shortest_index = search_shortest(num_of_process, All_Process, current_time);
if (shortest_index != -1) {
    if (processing_process_index != -1 && processing_process_index != shortest_index) {
        if (All_Process[processing_process_index].status != COMPLETED) {
            All_Process[processing_process_index].status = READY;
        }
    }
    if (All_Process[shortest_index].ResponseTime == 10000) {
        All_Process[shortest_index].ResponseTime = current_time;
        /* READY 表示の printf */
    }
    All_Process[shortest_index].status = RUNNING;
    Log[log_index++] = (GanttLog){ current_time,
                                   All_Process[shortest_index].ID,
                                   All_Process[shortest_index].RemainingBurstTime,
                                   RUNNING };
    tick_completed = 0;
    All_Process[shortest_index].run_permission = 1;
    pthread_cond_signal(&All_Process[shortest_index].my_cond);
    while (tick_completed == 0) {
        pthread_cond_wait(&scheduler_cond, &global_mutex);
    }
    if (All_Process[shortest_index].RemainingBurstTime == 0) {
        All_Process[shortest_index].status = COMPLETED;
        All_Process[shortest_index].CompletionTime = current_time + 1;
        Log[log_index++] = (GanttLog){ current_time + 1,
                                       All_Process[shortest_index].ID, 0, COMPLETED };
        completed_count++;
        /* COMPLETED 表示の printf */
    }
    processing_process_index = shortest_index;
} else {
    Log[log_index++] = (GanttLog){ current_time, -1, 0, READY };
    /* IDLE 表示 */
    processing_process_index = -1;
}
current_time++;
pthread_mutex_unlock(&global_mutex);
```

### `search_shortest` の実装
SRTF のコアは、到着済みで残り時間が最少のプロセスを見つける以下のループ。

```c
int search_shortest(int num_of_process, Process *All_Process, int current_time) {
    int shortest_index = -1;
    int min_remaining_time = 999999;
    for (int i = 0; i < num_of_process; i++) {
        if (All_Process[i].ArrivalTime <= current_time) {
            if (All_Process[i].RemainingBurstTime > 0) {
                if (All_Process[i].RemainingBurstTime < min_remaining_time) {
                    min_remaining_time = All_Process[i].RemainingBurstTime;
                    shortest_index = i;
                }
            }
        }
    }
    return shortest_index;
}
```

未到着のプロセスは `ArrivalTime <= current_time` が偽なので除外される。

### 5. シミュレーション終了処理
すべて完了すると `simulation_finished` を立て、待機中の全プロセスを起こし、`pthread_join`。続いてガントチャートと統計値を出力する。

```c
simulation_finished = 1;
for (int i = 0; i < num_of_process; i++) {
    pthread_cond_signal(&All_Process[i].my_cond);
}
for (int i = 0; i < num_of_process; i++) {
    pthread_join(All_Process[i].thread, NULL);
}
gantt_chart(Log, log_index);
```

---

## ガントチャート描画
`Log` を走査し、プロセス ID が変わる境目ごとに `| Pk |` を出力する。`process_id == -1` のブロックは `IDLE` として扱う。

```c
void gantt_chart(GanttLog *Log, int log_index) {
    int current_pid = Log[0].process_id;
    for (int i = 1; i < log_index; i++) {
        if (Log[i].status == COMPLETED) continue;
        if (Log[i].process_id != current_pid) {
            if (current_pid == -1) printf("|  IDLE ");
            else printf("|  P%-4d", current_pid);
            current_pid = Log[i].process_id;
        }
    }
    if (current_pid == -1) printf("|  IDLE |\n");
    else printf("|  P%-4d|\n", current_pid);
    /* 下段の時間軸表示も Log を走査して描画 */
}
```

ガントチャートは「プロセス境界のみに仕切りを描く」構成なので、プリエンプトが多いケースでも読みやすい。

---

## 表形式ログのフォーマット
メインループでは毎ティック、中央寄せの表形式で状態を出力する。幅は手作業で整えている。

```c
printf("%5s | %10s | %*s%s%*s |%*s%s%*s|\n",
       "Time", "Process ID", 3, "", "Status", 3, "",
       1, "", "Remaining Time", 1, "");
printf("------|------------|--------------|----------------|\n");

printf("%5d |%*s%s%*s|%*s%s%*s|%*s%s%*s|\n",
       current_time, l_pid, "", pid_str, r_pid, "",
       l_stat, "", stat_str, r_stat, "",
       l_rem, "", rem_str, r_rem, "");
```

`READY` 出力は初回実行時のみ、その後は `RUNNING` / `COMPLETED` / `IDLE` の行が 1 ティック毎に表示される。

---

## スレッド同期のポイント

```c
tick_completed = 0;
All_Process[idx].run_permission = 1;
pthread_cond_signal(&All_Process[idx].my_cond);
while (tick_completed == 0) {
    pthread_cond_wait(&scheduler_cond, &global_mutex);
}
```

1. **1 ティック実行モデル**: プロセスは `scheduler_cond` が解除されるまで 1 単位のみ進むので、プリエンプションの切り替えロジックは単純なフラグ操作で済む。
2. **条件変数の役割分担**: 個別の `my_cond` は実行許可の伝搬に、`scheduler_cond` は完了通知に使用し、役割が分かれているため待機が衝突しない。
3. **`simulation_finished` フラグ**: メインループ終了後は全スレッドを起こして抜けさせることでデッドロックを防ぐ。

---

## 実行結果と性能指標
完了後、各プロセスのターンアラウンド・待ち時間・応答時間を計算し、平均値を表示する。

```c
for (int i = 0; i < num_of_process; i++) {
    All_Process[i].TurnaroundTime =
        All_Process[i].CompletionTime - All_Process[i].ArrivalTime;
    All_Process[i].WaitingTime =
        All_Process[i].TurnaroundTime - All_Process[i].BurstTime;
    int response_time = All_Process[i].ResponseTime - All_Process[i].ArrivalTime;
    printf("Process %d: Turnaround = %d, Waiting = %d, Response = %d\n",
           All_Process[i].ID, All_Process[i].TurnaroundTime,
           All_Process[i].WaitingTime, response_time);
}
printf("Average Turnaround Time = %.2f\n", Avg_TAT / num_of_process);
printf("Average Waiting Time = %.2f\n", Avg_WT / num_of_process);
printf("Average Response Time = %.2f\n", Avg_RT / num_of_process);
```

`ResponseTime` は初回 RUNNING の時刻を記録しているため、SRTF の「応答性の良さ」を定量化できる。

---

## 応用・改変ヒント（該当部分の目安）
1. **時間粒度の変更**: `tick_completed` 周りを調整すれば「1 ティック = 1ms」以外の粒度や仮想的な仕事量を扱える。
2. **優先度付きキュー**: `search_shortest` は O(n) なので、プロセスが多い場合はヒープ構造に差し替える余地がある。
3. **I/O バースト対応**: `RemainingBurstTime` と別に I/O 待機時間を管理するフィールドを追加すれば、より複雑なワークロードも再現できる。

---

## まとめ
- POSIX スレッドを活用した SRTF 学習用サンプルであり、1 ティック単位でスケジューラとワーカースレッドが協調する設計を採用している。
- `Process` / `GanttLog` / 条件変数など、各データ構造と同期機構がどのように結合しているかを理解することで、プリエンプティブな CPU スケジューラの実装手順が把握できる。
- 実行ログ・ガントチャート・性能指標がセットで出力されるため、アルゴリズム比較やレポート作成の素材としてそのまま利用できる。
