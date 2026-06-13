

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(void)
{
    int i;
    printf("memory_hog: allocating 50 MB every 2 seconds...\n");
    printf("Press Ctrl+C to stop.\n\n");

    for (i = 1; i <= 20; i++) {
       
        char *block = (char *)malloc(50 * 1024 * 1024);
        if (!block) {
            fprintf(stderr, "malloc failed at iteration %d\n", i);
            break;
        }
        memset(block, 0xAB, 50 * 1024 * 1024);
        printf("  Allocated %4d MB total (PID %d)\n", i * 50, (int)getpid());
        sleep(2);
    }

    printf("\nHolding memory for 30 seconds so the detector can observe it...\n");
    sleep(30);
    return 0;
}
