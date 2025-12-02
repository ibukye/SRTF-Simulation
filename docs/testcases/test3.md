# Test Case 3 – Late Batch After Idle CPU (3 Processes)

## Input Parameters
| Process | Arrival | Burst |
|---------|---------|-------|
| P1 | 2 | 3 |
| P2 | 5 | 6 |
| P3 | 7 | 4 |

## Gantt Chart
```
|  IDLE |  P1   |  P2   |  P3   |
0       2       5       11      15    
```

## Performance Metrics
| Process | Turnaround | Waiting | Response |
|---------|------------|---------|----------|
| P1 | 3 | 0 | 0 |
| P2 | 6 | 0 | 0 |
| P3 | 8 | 4 | 4 |

Average Turnaround = **5.67**, Average Waiting = **1.33**, Average Response = **1.33**

## Observation
Because no process arrives until time 2, the CPU idles, then executes each job nearly back-to-back. The last process experiences the largest response and waiting times due to its late arrival into a busy system.
