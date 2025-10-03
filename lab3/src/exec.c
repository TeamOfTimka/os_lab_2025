#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    printf("Parent process: PID = %d\n", getpid());
    
    pid_t pid = fork();
    
    if (pid == -1) {
        perror("fork failed");
        return 1;
    }
    
    if (pid == 0) {
        printf("Child process: PID = %d, starting sequential_min_max\n", getpid());
        
        char *args[] = {
            "./sequential_min_max",  // имя программы
            "42",                    // seed
            "10",                    // array_size
            NULL                     // завершающий NULL
        };
        
        execvp(args[0], args);
        
        perror("execvp failed");
        exit(1);
    } else {
        printf("Parent process: waiting for child %d to finish\n", pid);
        wait(NULL);
        printf("Parent process: child finished\n");
    }
    
    return 0;
}