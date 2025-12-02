# Test Case 5 – Mixed Short/Long with Late Burst (5 Processes)

## Input Parameters
| Process | Arrival | Burst |
|---------|---------|-------|
| P1 | 0 | 3 |
| P2 | 1 | 8 |
| P3 | 2 | 6 |
| P4 | 3 | 4 |
| P5 | 10 | 2 |

## Gantt Chart
```
|  P1   |  P4   |  P3   |  P5   |  P3   |  P2   |
0       3       7       10      12      15      23   
```

## Performance Metrics
| Process | Turnaround | Waiting | Response |
|---------|------------|---------|----------|
| P1 | 3 | 0 | 0 |
| P2 | 22 | 14 | 14 |
| P3 | 13 | 7 | 5 |
| P4 | 4 | 0 | 0 |
| P5 | 2 | 0 | 0 |

Average Turnaround = **8.80**, Average Waiting = **4.20**, Average Response = **3.80**

## Observation
The late short job P5 preempts the ongoing medium job (P3), but once all short work is done the long job P2 finally runs, illustrating how late-arriving short jobs can further postpone already waiting long bursts.
