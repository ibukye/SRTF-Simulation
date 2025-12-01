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
