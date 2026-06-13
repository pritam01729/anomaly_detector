
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>   
#include <time.h>        
#include <signal.h>    
#include "collector.h"
#include "detector.h"
#include "utils.h"


#define INTERVAL_SEC 3
#define TOP_N        15
//colour codes
#define RED    "\033[0;31m"
#define YELLOW "\033[0;33m"
#define GREEN  "\033[0;32m"
#define CYAN   "\033[0;36m"
#define RESET  "\033[0m"
#define BOLD   "\033[1m"
#define CLEAR  "\033[2J\033[H"


static volatile int running = 1;

static void handle_sigint(int sig)
{
    (void)sig;
    running = 0;
}


static double now_sec(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

static void sort_by_cpu(Process *arr, int n)
{
    int i, j;
    for (i = 0; i < n - 1; i++) {
        for (j = i + 1; j < n; j++) {
            if (arr[j].cpu_percent > arr[i].cpu_percent) {
                Process tmp = arr[i];
                arr[i] = arr[j];
                arr[j] = tmp;
            }
        }
    }
}

int main(void)
{
    signal(SIGINT, handle_sigint);

    Process prev[MAX_PROCESSES], curr[MAX_PROCESSES];
    int     prev_count = 0, curr_count = 0;
    double  prev_time  = 0, curr_time  = 0;
    int     round_num  = 0;

    //Write session marker to log
    {
        FILE *lf = fopen("anomalies.log", "a");
        if (lf) {
            time_t t = time(NULL);
            char   ts[32];
            strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", localtime(&t));
            fprintf(lf, "\n=== Session started at %s ===\n", ts);
            fclose(lf);
        }
    }

    printf(BOLD "Anomaly Detector starting...\n" RESET);
    printf("Building baseline — first %d rounds will be quiet.\n\n", MIN_SAMPLES);

    // First snapshot so we have a "previous" for the CPU delta 
    prev_time  = now_sec();
    prev_count = collect_processes(prev, MAX_PROCESSES);
    sleep(INTERVAL_SEC);

    while (running) {
        int i, j;
        curr_time  = now_sec();
        curr_count = collect_processes(curr, MAX_PROCESSES);
        double elapsed = curr_time - prev_time;
        round_num++;

        FILE *lf = fopen("anomalies.log", "a");

        //Compute CPU% and check for anomalies
        for (i = 0; i < curr_count; i++) {
            Process *c = &curr[i];

            // Match against previous snapshot to get tick delta 
            float cpu_pct = 0.0f;
            for (j = 0; j < prev_count; j++) {
                if (prev[j].pid == c->pid) {
                    long delta = (c->utime + c->stime)
                               - (prev[j].utime + prev[j].stime);
                    long tps   = sysconf(_SC_CLK_TCK);
                    cpu_pct = (float)delta / (float)tps / (float)elapsed * 100.0f;
                    if (cpu_pct < 0.0f) cpu_pct = 0.0f;
                    break;
                }
            }
            c->cpu_percent = cpu_pct;

            // Update history and run detector 
            ProcessHistory *h = find_or_create_history(c->pid, c->name);
            if (!h) continue;
            update_history(h, c->cpu_percent, c->mem_mb);

            Anomaly a;
            if (check_anomaly(h, c, &a)) {
                printf(RED BOLD "\n[!] ANOMALY  PID %-6d  %-20s\n" RESET,
                       a.pid, a.name);
                printf(RED "    %s\n" RESET, a.reason);

                if (lf) {
                    time_t t = time(NULL);
                    char   ts[32];
                    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", localtime(&t));
                    fprintf(lf, "[%s] PID=%-6d %-20s %s\n",
                            ts, a.pid, a.name, a.reason);
                }
            }

         //zombie alert
            if (c->state == 'Z')
                printf(YELLOW "[!] ZOMBIE   PID %-6d  %-20s\n" RESET,
                       c->pid, c->name);
        }

        if (lf) fclose(lf);

        printf(CLEAR);
        printf(BOLD CYAN "%-6s  %-20s  %6s  %8s  %7s  %s\n" RESET,
               "PID", "Name", "CPU %", "Mem MB", "Threads", "State");
        printf("──────────────────────────────────────────────────────\n");

        sort_by_cpu(curr, curr_count);

        int show = curr_count < TOP_N ? curr_count : TOP_N;
        for (i = 0; i < show; i++) {
            Process    *p     = &curr[i];
            const char *color = p->cpu_percent > 50.0f ? RED  :
                                p->cpu_percent > 20.0f ? YELLOW : GREEN;
            printf("%s%-6d  %-20.20s  %5.1f%%  %7.1fMB  %7d  %c%s\n",
                   color, p->pid, p->name,
                   p->cpu_percent, p->mem_mb, p->num_threads,
                   p->state, RESET);
        }

        printf("\n" BOLD
               "Round: %d | Processes: %d | Interval: %ds | Ctrl+C to stop\n"
               RESET, round_num, curr_count, INTERVAL_SEC);

        //swap snapshots
        memcpy(prev, curr, (size_t)curr_count * sizeof(Process));
        prev_count = curr_count;
        prev_time  = curr_time;

        sleep(INTERVAL_SEC);
    }

    printf("\n\nStopped. Check anomalies.log for the full alert history.\n");
    return 0;
}
