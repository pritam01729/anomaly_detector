/*
 * collector.c
 * Scans /proc, reads each process's stat and status files,
 * and fills an array of Process structs(since the details are not stored by default, kernel only shows when I use proc)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>    
#include <ctype.h>     
#include <unistd.h>   
#include "collector.h"

static long TICKS_PER_SEC = 0;

/*
 * Read CPU ticks and process state from /proc/<pid>/stat.
 * The name field is wrapped in parentheses and can contain spaces,
 * so I need to locate the last ')' before parsing the remaining fields.
 */
static int read_stat(int pid, Process *p)
{
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);

    FILE *f = fopen(path, "r");
    if (!f) return 0;

    char raw[512];
    if (!fgets(raw, sizeof(raw), f)) { fclose(f); return 0; }
    fclose(f);

    /* Find the name between the first '(' and last ')' */
    char *name_start = strchr(raw,  '(');
    char *name_end   = strrchr(raw, ')');
    if (!name_start || !name_end) return 0;

    int name_len = (int)(name_end - name_start - 1);
    if (name_len >= MAX_NAME_LEN) name_len = MAX_NAME_LEN - 1;
    strncpy(p->name, name_start + 1, name_len);
    p->name[name_len] = '\0';

    /* Parse fields after ')': state(3) ppid(4)..flags(9-13) utime(14) stime(15) */
    //also I dont need the inbetween, those will be taken as dummies
    char state;
    long utime, stime;
    int  di; long dl;

    sscanf(name_end + 2,
        "%c "
        "%d %d %d %d %d "
        "%u %lu %lu %lu %lu "
        "%lu %lu",
        &state,
        &di, &di, &di, &di, &di,
        (unsigned int *)&dl, &dl, &dl, &dl, &dl,
        &utime, &stime);

    p->state = state;
    p->utime = utime;
    p->stime = stime;
    return 1;
}


 //Read memory usage and thread count from /proc/<pid>/status.
 
 
static int read_status(int pid, Process *p)
{
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);

    FILE *f = fopen(path, "r");
    if (!f) return 0;

    char line[128];
    long vmrss_kb = 0;
    int  threads  = 1;

    while (fgets(line, sizeof(line), f)) {
        if      (strncmp(line, "VmRSS:",   6) == 0)
            sscanf(line, "VmRSS: %ld kB", &vmrss_kb);
        else if (strncmp(line, "Threads:", 8) == 0)
            sscanf(line, "Threads: %d",   &threads);
    }
    fclose(f);

    p->mem_mb      = vmrss_kb / 1024.0f;
    p->num_threads = threads;
    return 1;
}

/*
 Scan /proc for all running processes.
  Returns the number of processes successfully collected.
 */
int collect_processes(Process *procs, int max_count)
{
    if (TICKS_PER_SEC == 0)
        TICKS_PER_SEC = sysconf(_SC_CLK_TCK); 

    DIR *proc_dir = opendir("/proc");
    if (!proc_dir) return 0;

    int count = 0;
    struct dirent *entry;

    while ((entry = readdir(proc_dir)) != NULL && count < max_count) {
        if (!isdigit((unsigned char)entry->d_name[0])) continue;

        int pid = atoi(entry->d_name);
        Process p;
        memset(&p, 0, sizeof(p));
        p.pid = pid;

        if (!read_stat(pid,   &p)) continue; //if the process dies
        if (!read_status(pid, &p)) continue;

        procs[count++] = p;
    }

    closedir(proc_dir);
    return count;
}
