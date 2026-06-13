//helper functions

#include <stdio.h>
#include <time.h>
#include "utils.h"

/* Print the current wall-clock time as [HH:MM:SS] */
void print_timestamp(void)
{
    time_t     t  = time(NULL);
    struct tm *tm = localtime(&t);
    char       buf[32];
    strftime(buf, sizeof(buf), "%H:%M:%S", tm);
    printf("[%s] ", buf);
}
 