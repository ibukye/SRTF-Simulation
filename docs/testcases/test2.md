# Test Case 2 – Dense Early Arrivals (5 Processes)

## Input Parameters
| Process | Arrival | Burst |
|---------|---------|-------|
| P1 | 0 | 9 |
| P2 | 1 | 4 |
| P3 | 2 | 2 |
| P4 | 3 | 7 |
| P5 | 4 | 3 |

## Gantt Chart
```
|  P1   |  P2   |  P3   |  P2   |  P5   |  P4   |  P1   |
0       1       2       4       7       10      17      25    
```

## Performance Metrics
| Process | Turnaround | Waiting | Response |
|---------|------------|---------|----------|
| P1 | 25 | 16 | 0 |
| P2 | 6 | 2 | 0 |
| P3 | 2 | 0 | 0 |
| P4 | 14 | 7 | 7 |
| P5 | 6 | 3 | 3 |

Average Turnaround = **10.60**, Average Waiting = **5.60**, Average Response = **2.00**

## Observation
Closely spaced arrivals let shorter bursts (P2–P5) repeatedly displace the long initial job (P1); as a result, throughput remains high but starvation risk surfaces for long jobs when short ones keep arriving.
