#ifndef COLLECTOR_H
#define COLLECTOR_H

#define MAX_NAME_LEN  64
#define MAX_PROCESSES 1024

typedef struct {
    int   pid;
    char  name[MAX_NAME_LEN];
    char  state;           // R=running, S=sleeping, Z=zombie, D=uninterruptible 
    float cpu_percent;     
    float mem_mb;         
    int   num_threads;
    long  utime;           
    long  stime;          
} Process;

int collect_processes(Process *procs, int max_count);

#endif
