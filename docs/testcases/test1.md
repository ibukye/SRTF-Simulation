# Test Case 1 – Baseline Mix (4 Processes)

## Input Parameters
| Process | Arrival | Burst |
|---------|---------|-------|
| P1 | 0 | 7 |
| P2 | 2 | 4 |
| P3 | 4 | 1 |
| P4 | 5 | 4 |

## Gantt Chart
```
|  P1   |  P2   |  P3   |  P2   |  P4   |  P1   |
0       2       4       5       7       11      16     
```

## Performance Metrics
| Process | Turnaround | Waiting | Response |
|---------|------------|---------|----------|
| P1 | 16 | 9 | 0 |
| P2 | 5 | 1 | 0 |
| P3 | 1 | 0 | 0 |
| P4 | 6 | 2 | 2 |

Average Turnaround = **7.00**, Average Waiting = **3.00**, Average Response = **0.50**

## Observation
Early-arriving long job P1 suffers from repeated preemption by shorter jobs that appear later, which increases its waiting and turnaround times while improving latency for P2–P4.
