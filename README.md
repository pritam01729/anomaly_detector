# Anomaly Detection in System Processes

A real-time C program that monitors all running Linux processes and flags
anomalous behaviour using Z-score statistical analysis.

## How it works

1. Reads `/proc/<pid>/stat` and `/proc/<pid>/status` for every running process
2. Maintains a circular buffer of the last 30 readings per process
3. Computes Z-score: `Z = |value - mean| / stdev`
4. Flags processes where `Z > 2.5` as anomalies
5. Detects zombie processes (state = 'Z') immediately

## Build

```bash
make          # build the detector
make sim      # also build the simulation helpers
```

## Run

```bash
sudo ./anomaly_detector
```

`sudo` is needed to read `/proc` entries owned by other users.

## Simulate anomalies (open a second terminal)

```bash
# CPU spike (requires the 'stress' package)
stress --cpu 2 --timeout 30

# Memory spike
./simulate/memory_hog

# Zombie process
./simulate/zombie &
ps aux | grep " Z "
```

## Watch the log in real time

```bash
tail -f anomalies.log
```

## File layout

```
anomaly_detector/
├── Makefile
├── README.md
├── anomalies.log          (created at runtime)
├── src/
│   ├── main.c             monitor loop, dashboard, logging
│   ├── collector.c        reads /proc, fills Process structs
│   ├── collector.h
│   ├── detector.c         Z-score engine, circular buffer
│   ├── detector.h
│   ├── utils.c            helper functions
│   └── utils.h
└── simulate/
    ├── memory_hog.c       allocates RAM in chunks to trigger mem anomaly
    └── zombie.c           creates a zombie process to trigger state alert
```

## Further improvement scope
```
We have used a very basic normal distribution and Z-score to detect the outliers. 
There are certain limitations because of this (like monotonic increase in memory space for a process can't be detected).
Also we have used only a few parameters to pick the anomalies. Which can be improved further if we take account of
other paramters. 
One mazor update would be to use offline ML models to train on previous history of processes and then we can detect anomalies 
based on the live prediction of the ML model during runnning processes. 
```
