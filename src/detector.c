/*
  Maintains per-process history buffers and runs Z-score anomaly detection.
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "detector.h"
//Global store — linear array
static ProcessHistory histories[MAX_PROCESSES];
static int            history_count = 0;

/*
  Return the history entry for this pid, creating one if it does not exist.
  Returns NULL if the table is full.
 */
ProcessHistory *find_or_create_history(int pid, const char *name)
{
    int i;
    for (i = 0; i < history_count; i++)
        if (histories[i].pid == pid) return &histories[i];

    if (history_count >= MAX_PROCESSES) return NULL;

    ProcessHistory *h = &histories[history_count++];
    memset(h, 0, sizeof(*h));
    h->pid = pid;
    strncpy(h->name, name, MAX_NAME_LEN - 1);
    return h;
}

//circular buffer
void update_history(ProcessHistory *h, float cpu, float mem)
{
    h->cpu_history[h->head] = cpu;
    h->mem_history[h->head] = mem;
    h->head = (h->head + 1) % HISTORY_SIZE; 
    if (h->count < HISTORY_SIZE) h->count++;
}

float compute_mean(float *arr, int n)
{
    float sum = 0.0f;
    int   i;
    for (i = 0; i < n; i++) sum += arr[i];
    return sum / (float)n;
}


float compute_stdev(float *arr, int n, float mean)
{
    float variance = 0.0f;
    int   i;
    for (i = 0; i < n; i++) {
        float d = arr[i] - mean;
        variance += d * d;
    }
    return sqrtf(variance / (float)n);
}

int check_anomaly(ProcessHistory *h, Process *p, Anomaly *out)
{
    if (h->count < MIN_SAMPLES) return 0; 

    float cpu_mean  = compute_mean(h->cpu_history,  h->count);
    float mem_mean  = compute_mean(h->mem_history,  h->count);
    float cpu_stdev = compute_stdev(h->cpu_history, h->count, cpu_mean);
    float mem_stdev = compute_stdev(h->mem_history, h->count, mem_mean);

    
    if (cpu_stdev < 0.1f) cpu_stdev = 0.1f;
    if (mem_stdev < 0.1f) mem_stdev = 0.1f;

    float cpu_z = fabsf((p->cpu_percent - cpu_mean) / cpu_stdev);
    float mem_z = fabsf((p->mem_mb     - mem_mean)  / mem_stdev);

    int  is_anomaly = 0;
    char buf[128];
    out->reason[0] = '\0';

    if (cpu_z > Z_THRESHOLD) {
        snprintf(buf, sizeof(buf),
            "CPU spike: %.1f%% (mean=%.1f%%, z=%.2f)  ",
            p->cpu_percent, cpu_mean, cpu_z);
        strncat(out->reason, buf, sizeof(out->reason) - strlen(out->reason) - 1);
        is_anomaly = 1;
    }
    if (mem_z > Z_THRESHOLD) {
        snprintf(buf, sizeof(buf),
            "Mem spike: %.1fMB (mean=%.1fMB, z=%.2f)  ",
            p->mem_mb, mem_mean, mem_z);
        strncat(out->reason, buf, sizeof(out->reason) - strlen(out->reason) - 1);
        is_anomaly = 1;
    }
    if (p->state == 'Z') {
        strncat(out->reason, "Zombie process detected!",
                sizeof(out->reason) - strlen(out->reason) - 1);
        is_anomaly = 1;
    }

    if (is_anomaly) {
        out->pid   = p->pid;
        out->cpu   = p->cpu_percent;
        out->mem   = p->mem_mb;
        out->cpu_z = cpu_z;
        out->mem_z = mem_z;
        strncpy(out->name, p->name, MAX_NAME_LEN - 1);
    }

    return is_anomaly;
}
