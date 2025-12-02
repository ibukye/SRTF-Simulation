# Refactoring Report

## Step 1: Fix Magic Numbers
- **Issue**: The code used `999999` for minimum remaining time initialization and `10000` for uninitialized response time. This is risky as valid values could exceed these limits or conflict with them.
- **Fix**: 
    - Included `<limits.h>`.
    - Replaced `999999` with `INT_MAX`.
    - Replaced `10000` with `-1` (indicating "not set").
- **Status**: Implemented.

## Step 2: Fix Code Duplication (Printing)
- **Issue**: The code contained 4 identical blocks of `printf` logic for the progression table, making it hard to read and maintain.
- **Fix**: 
    - Created a helper function `print_row(int time, const char* pid_str, const char* status_str, const char* rem_str)`.
    - Replaced the duplicated blocks with calls to `print_row`.
- **Status**: Implemented.

## Step 3: Refactor Main Function
- **Issue**: The `main` function was over 200 lines long, handling initialization, scheduling, and reporting. This violated the Single Responsibility Principle.
- **Fix**: 
    - Extracted scheduling logic into `run_scheduler(int num_processes, Process *processes)`.
    - Extracted reporting logic into `print_performance_metrics(int num_processes, Process *processes)`.
    - Simplified `main` to coordinate these functions.
- **Status**: Implemented.

## Step 4: Variable Naming & Cleanup
- **Issue**: Variable names like `All_Process` (singular/inconsistent) and `num_of_process` were not idiomatic. Unused headers were present.
- **Fix**: 
    - Renamed `All_Process` to `processes`.
    - Renamed `num_of_process` to `num_processes`.
    - Removed unused `<ctype.h>`.
- **Status**: Implemented.

## Step 5: Design Refactoring (Context & Enums)
- **Issue**: Global variables (`global_mutex`, `Log`, etc.) made the code hard to test and maintain. State was represented by macros (`#define READY 0`) lacking type safety.
- **Fix**: 
    - Introduced `SimulationContext` struct to hold all shared state.
    - Introduced `process_state_t` enum for type-safe state management.
    - Updated functions to accept `SimulationContext*` instead of relying on globals.
- **Status**: Implemented.

## Step 6: Memory Management
- **Issue**: `Process processes[num_processes]` used Variable Length Arrays (VLA), which can cause stack overflow and are not standard in all C versions (e.g., MSVC).
- **Fix**: 
    - Changed to dynamic allocation using `calloc`.
    - Added `free` at the end of `main`.
- **Status**: Implemented.

## Step 7: Input/Output Cleanup
- **Issue**: `scanf` loops were repetitive and fragile. `sprintf` was unsafe.
- **Fix**: 
    - Created `read_int` helper function using `fgets` and `strtol` for robust input parsing.
    - Replaced `sprintf` with `snprintf` to prevent buffer overflows.
    - Defined constants for column widths (`COL_WIDTH_PID`, etc.).
- **Status**: Implemented.

## Step 8: Synchronization & Error Handling (Robustness)
- **Issue**: System calls (`pthread_mutex_lock`, etc.) were not checked for errors. Thread creation failure caused resource leaks. Log buffer overflow was ignored.
- **Fix**: 
    - Added `check_pthread` helper to validate all thread-related calls.
    - Implemented `goto cleanup` pattern in `run_scheduler` to safely release resources on failure.
    - Added explicit overflow check in `add_log`.
    - Added design justification comments for the "lock-step" synchronization model.
- **Status**: Implemented.

