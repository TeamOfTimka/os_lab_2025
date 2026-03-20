#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork();

    if (pid == 0) {
        printf("[Child] My PID: %d. I'm exiting now...\n", getpid());
        exit(0);
    } else if (pid > 0) {
        printf("[Parent] My PID: %d. I created child %d.\n", getpid(), pid);
        printf("[Parent] Sleeping for 5 seconds. Check the process state with 'ps aux | grep Z'.\n");
        printf("[Parent] The child (PID %d) should be in 'Z' (zombie) state.\n", pid);
        sleep(5);

        printf("[Parent] Waking up and calling wait() to clean up the zombie.\n");
        wait(NULL);
        printf("[Parent] Child cleaned up. Check process list again.\n");
    } else {
        perror("fork");
        return 1;
    }
    return 0;
}