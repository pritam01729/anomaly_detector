#ifndef DETECTOR_H
#define DETECTOR_H

#include "collector.h"

#define HISTORY_SIZE  30   //last 30 readings
#define MIN_SAMPLES    5    //this is for the coold period
#define Z_THRESHOLD  2.5f  


typedef struct {
    int   pid;
    char  name[MAX_NAME_LEN];
    float cpu_history[HISTORY_SIZE];
    float mem_history[HISTORY_SIZE];
    int   count; //readings collected so far 
    int   head; 
} ProcessHistory;


typedef struct {
    int   pid;
    char  name[MAX_NAME_LEN];
    float cpu;
    float mem;
    float cpu_z;
    float mem_z;
    char  reason[256];
} Anomaly;

ProcessHistory *find_or_create_history(int pid, const char *name);
void            update_history(ProcessHistory *h, float cpu, float mem);
int             check_anomaly(ProcessHistory *h, Process *p, Anomaly *out);
float           compute_mean(float *arr, int n);
float           compute_stdev(float *arr, int n, float mean);

#endif
