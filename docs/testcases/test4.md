# Test Case 4 – High Preemption with Long Tail (5 Processes)

## Input Parameters
| Process | Arrival | Burst |
|---------|---------|-------|
| P1 | 0 | 8 |
| P2 | 1 | 4 |
| P3 | 3 | 9 |
| P4 | 6 | 2 |
| P5 | 8 | 5 |

## Gantt Chart
```
|  P1   |  P2   |  P1   |  P4   |  P5   |  P1   |  P3   |
0       1       5       6       8       13      19      28   
```

## Performance Metrics
| Process | Turnaround | Waiting | Response |
|---------|------------|---------|----------|
| P1 | 19 | 11 | 0 |
| P2 | 4 | 0 | 0 |
| P3 | 25 | 16 | 16 |
| P4 | 2 | 0 | 0 |
| P5 | 5 | 0 | 0 |

Average Turnaround = **11.00**, Average Waiting = **5.40**, Average Response = **3.20**

## Observation
Short bursts that appear mid-schedule (P4, P5) immediately preempt longer tasks, showing how SRTF maximizes responsiveness but delays heavy processes (P3 waited 16 units before running).
