# SRTF Simulation (Multi-Threaded)

## Overview
This project implements a **Shortest Remaining Time First (SRTF)** process scheduling simulation using C and POSIX Threads (`pthread`). It demonstrates the core concepts of preemptive scheduling, thread synchronization, and critical section management.

## Build & Run

### Prerequisites
- GCC Compiler (supporting C99 or later)
- POSIX Threads library (`libpthread`)
- Linux/Unix environment (or WSL on Windows)

### Compilation
Use the following command to compile the program:
```bash
gcc -std=c17 -Wall -Wextra -g MultiThreading.c -o srtf -lpthread
```

### Execution
Run the compiled executable:
```bash
./srtf
```
Follow the on-screen prompts to enter the number of processes (1-10) and their Arrival/Burst times.

## Design Report

### 1. Algorithmic Approach
The simulation uses the **SRTF (Shortest Remaining Time First)** algorithm, which is the preemptive version of SJF (Shortest Job First).
- **Process Selection**: At each time unit (tick), the scheduler checks all ready processes.
- **Decision Logic**: It selects the process with the **minimum remaining burst time**.
- **Preemption**: If a new process arrives with a shorter burst time than the current running process, the scheduler preempts the current one.
- **Complexity**: The current implementation uses a linear scan ($O(N)$) to find the shortest job. For $N \le 10$, this is computationally negligible and simplifies the state management compared to a Min-Heap ($O(\log N)$).

### 2. Threading Model: "Lock-Step" Synchronization
The simulation employs a **Lock-Step** synchronization model to ensure deterministic behavior and accurate time tracking.
- **Why not true parallelism?** In a real OS, the scheduler and processes run asynchronously. However, simulating exact 1-second CPU bursts in real-time with threads is non-deterministic due to OS scheduling jitter.
- **Implementation**:
    1.  **Scheduler Thread**: Decides which process runs, signals that specific thread, and waits.
    2.  **Worker Thread**: Wakes up, decrements its remaining time by 1 (simulating 1 tick), signals the scheduler back, and waits.
    3.  This "ping-pong" signaling ensures that exactly one unit of work is performed per simulation tick, guaranteeing the Gantt chart is perfectly accurate.

### 3. Architecture & Robustness
- **SimulationContext**: All shared state (mutexes, condition variables, logs) is encapsulated in a `SimulationContext` structure, avoiding global variables.
- **Type Safety**: Process states are managed using a `process_state_t` enum (`READY`, `RUNNING`, `COMPLETED`) rather than integer magic numbers.
- **Error Handling**: All system calls (`pthread_create`, `mutex_lock`, etc.) are wrapped in a `check_pthread` helper that terminates the program safely with an error message upon failure.
- **Memory Safety**: Dynamic memory allocation (`calloc`) is used for process structures, and a `goto cleanup` pattern ensures resources are freed even in error paths.

### 4. Trade-offs
- **Linear Search vs. Heap**: We chose linear search for simplicity and code maintainability given the small constraint ($N \le 10$). A Heap would scale better for $N > 1000$ but adds significant complexity to handle preemption and state updates.
- **Simulation vs. Real-time**: The lock-step model trades execution speed for simulation accuracy. It does not utilize multi-core parallelism for performance but uses threads to model the *structure* of concurrent processes.
