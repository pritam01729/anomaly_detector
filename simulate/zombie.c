
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
    pid_t child = fork();

    if (child < 0) {
        perror("fork");
        return 1;
    }

    if (child == 0) {
       
        printf("Child  PID=%d: exiting now — becoming zombie...\n",
               (int)getpid());
        exit(0);
    } else {
      
        printf("Parent PID=%d: ignoring child PID=%d.\n",
               (int)getpid(), (int)child);
        printf("Verify zombie with:  ps aux | grep \" Z \"\n");
        printf("Parent will exit in 60 seconds...\n");
        sleep(60);
    }

    return 0;
}
