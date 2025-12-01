# Refactoring Report

## Step 1: Fix Magic Numbers
- **Issue**: The code used `999999` for minimum remaining time initialization and `10000` for uninitialized response time. This is risky as valid values could exceed these limits or conflict with them.
- **Fix**: 
    - Included `<limits.h>`.
    - Replaced `999999` with `INT_MAX`.
    - Replaced `10000` with `-1` (indicating "not set").
- **Status**: Implemented.
