# SRTF Simulation: Design & Implementation Report

## 1. Algorithmic Design & Justification

### 1.1 Algorithm Overview
This project implements the **Shortest Remaining Time First (SRTF)** scheduling algorithm. SRTF is the preemptive version of Shortest Job First (SJF).
- **Logic**: At every time unit (tick), the scheduler evaluates all available processes and selects the one with the strictly lowest `RemainingBurstTime`.
- **Preemption**: If a newly arrived process has a shorter remaining time than the currently running process, the scheduler preempts the current process and switches context.

### 1.2 Implementation: Linear Search vs. Min-Heap
The current implementation uses a **Linear Search ($O(N)$)** to identify the next process to run.

**Why Linear Search?**
While a Min-Heap (Priority Queue) offers $O(\log N)$ efficiency for finding the minimum element, we deliberately chose a linear search for the following reasons:

1.  **Constraints**: The coursework specifies a maximum of $N=10$ processes. At this scale, the performance difference between $O(N)$ and $O(\log N)$ is measured in nanoseconds and is effectively negligible.
2.  **Simplicity (KISS Principle)**: A Min-Heap adds significant complexity, particularly for **preemption** and **updates**. In SRTF, the `RemainingBurstTime` of the running process changes every tick. Updating a key in a binary heap requires re-heapifying, which complicates the data structure logic.
3.  **Robustness**: A linear scan is stateless and less prone to bugs (e.g., corrupted heap invariants). Given the small $N$, the maintenance overhead of a Heap outweighs its algorithmic benefits.

**Conclusion**: The linear search provides the optimal balance of correctness, maintainability, and performance for the given constraints.

## 2. Concurrency Model: "Lock-Step" Synchronization

### 2.1 The Challenge of Real-Time Simulation
In a real Operating System, the scheduler and processes run asynchronously, often triggered by hardware interrupts. However, simulating this behavior using standard user-space threads (`pthread`) introduces **non-determinism**. The OS scheduler might pause a thread for slightly longer than intended, causing the simulation's "1 second" to drift, leading to inaccurate Gantt charts.

### 2.2 The Solution: Lock-Step Model
To guarantee precision, we implemented a **Lock-Step Synchronization** model using `pthread_cond_t` and `pthread_mutex_t`.

1.  **Scheduler Turn**: The scheduler thread selects a process, signals that specific process's thread, and then **waits**.
2.  **Process Turn**: The signaled process thread wakes up, performs exactly one unit of work (decrements burst time), signals the scheduler back, and **waits**.
3.  **Deterministic Tick**: This "ping-pong" handshaking ensures that exactly one simulation tick occurs for every cycle, regardless of the host machine's speed or load.

This model prioritizes **simulation accuracy** over raw execution speed, ensuring the output is always theoretically correct.

## 3. Robustness & Safety Measures

### 3.1 Elimination of Global State
We replaced global variables with a `SimulationContext` structure.
- **Why**: Global variables make code brittle, hard to test, and prone to race conditions.
- **Implementation**: All shared state (mutexes, logs, flags) is encapsulated in `SimulationContext`, which is passed by pointer to threads. This improves modularity and encapsulation.

### 3.2 Fail-Fast Error Handling
All POSIX thread functions (`pthread_create`, `pthread_mutex_lock`, etc.) are wrapped in a `check_pthread` helper function.
- **Strategy**: If a critical system call fails (e.g., unable to create a thread), the program prints a descriptive error message and exits immediately (`exit(EXIT_FAILURE)`).
- **Justification**: Continuing execution after a synchronization primitive failure would lead to undefined behavior or deadlocks. A "fail-fast" approach is safer for this type of simulation.

### 3.3 Input Validation
User input is handled via a robust `read_int` function.
- **Features**: It prevents buffer overflows (using `fgets`), handles non-numeric input gracefully, and enforces range checks (e.g., Arrival Time $\ge 0$). This prevents the "garbage in, garbage out" scenarios common with raw `scanf`.

### 3.4 IDLE State Representation
The simulation logs CPU idle periods using `STATE_READY` with a special Process ID of `-1`.
- **Design Choice**: We avoided adding a specific `STATE_IDLE` to the `process_state_t` enum to keep the enum strictly focused on *process* states. IDLE is a system state, not a process state.
- **Trade-off**: This simplifies the state machine but requires the logger to check for `PID == -1` to identify idle periods.

## 4. Execution & Interface

### 4.1 Build Instructions
The project is standard C17 and uses the pthread library.

**Compile:**
```bash
gcc -std=c17 -Wall -Wextra -g MultiThreading.c -o srtf -lpthread
```

**Run:**
```bash
./srtf
```

### 4.2 Interface Features
- **Visual Gantt Chart**: A text-based Gantt chart visualizes the timeline, clearly showing context switches and idle periods.
- **Formatted Logging**: The progression table uses fixed-width columns (`%*s` padding) to ensure alignment regardless of the number of digits in the time or PID.
- **Performance Metrics**: Calculates and displays Average Turnaround Time, Waiting Time, and Response Time at the end of the simulation.
