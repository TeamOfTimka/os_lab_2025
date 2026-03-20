#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

pid_t *child_pids = NULL;
int children_count = 0;
int timeout_value = 0;

// Обработчик сигнала таймаута
void timeout_handler(int sig) {
    if (sig == SIGALRM && timeout_value > 0) {
        printf("\n[Parent] Timeout (%d seconds) reached! Killing all child processes...\n", timeout_value);
        for (int i = 0; i < children_count; i++) {
            if (child_pids[i] > 0) {
                if (kill(child_pids[i], SIGKILL) == 0) {
                    printf("[Parent] Killed child process %d\n", child_pids[i]);
                } else {
                    printf("[Parent] Failed to kill child process %d\n", child_pids[i]);
                }
            }
        }
        printf("[Parent] All children terminated due to timeout.\n");
    }
}

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  bool with_files = false;
  int timeout = -1;

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"seed", required_argument, 0, 0},
                                      {"array_size", required_argument, 0, 0},
                                      {"pnum", required_argument, 0, 0},
                                      {"by_files", no_argument, 0, 'f'},
                                      {"timeout", required_argument, 0, 0},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            seed = atoi(optarg);
            if (seed <= 0) {
              printf("seed must be a positive number\n");
              return 1;
            }
            break;
          case 1:
            array_size = atoi(optarg);
            if (array_size <= 0) {
              printf("array_size must be a positive number\n");
              return 1;
            }
            break;
          case 2:
            pnum = atoi(optarg);
            if (pnum <= 0) {
              printf("pnum must be a positive number\n");
              return 1;
            }
            break;
          case 3:
            with_files = true;
            break;
          case 4: // Новая опция timeout
            timeout = atoi(optarg);
            if (timeout <= 0) {
              printf("timeout must be a positive number\n");
              return 1;
            }
            break;

          default:
            printf("Index %d is out of options\n", option_index);
        }
        break;
      case 'f':
        with_files = true;
        break;

      case '?':
        break;

      default:
        printf("getopt returned character code 0%o?\n", c);
    }
  }

  if (optind < argc) {
    printf("Has at least one no option argument\n");
    return 1;
  }

  if (seed == -1 || array_size == -1 || pnum == -1) {
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" [--timeout \"num\"]\n",
           argv[0]);
    return 1;
  }

  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);
  int active_child_processes = 0;

  int pipefd[2 * pnum];

  if (!with_files) {
    for (int i = 0; i < pnum; i++) {
      if (pipe(pipefd + 2 * i) < 0) {
        printf("Pipe failed\n");
        return 1;
      }
    }
  }

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  // Выделяем память для хранения PID дочерних процессов
  child_pids = malloc(sizeof(pid_t) * pnum);
  children_count = pnum;

  for (int i = 0; i < pnum; i++) {
    pid_t child_pid = fork();
    if (child_pid >= 0) {
      // successful fork
      active_child_processes += 1;
      if (child_pid == 0) {
        // child process
        int chunk_size = array_size / pnum;
        int start = i * chunk_size;
        int end = (i == pnum - 1) ? array_size : (i + 1) * chunk_size;
        
        struct MinMax local_min_max = GetMinMax(array, start, end);

        if (with_files) {
          char filename[30];
          sprintf(filename, "min_max_%d.txt", i);
          FILE *file = fopen(filename, "w");
          fprintf(file, "%d %d", local_min_max.min, local_min_max.max);
          fclose(file);
        } else {
          close(pipefd[2 * i]);
          write(pipefd[2 * i + 1], &local_min_max.min, sizeof(int));
          write(pipefd[2 * i + 1], &local_min_max.max, sizeof(int));
          close(pipefd[2 * i + 1]);
        }
        return 0;
      } else {
        child_pids[i] = child_pid;
      }
    } else {
      printf("Fork failed!\n");
      return 1;
    }
  }

  // Устанавливаем обработчик таймаута, если задан timeout
  if (timeout > 0) {
    timeout_value = timeout;
    signal(SIGALRM, timeout_handler);
    alarm(timeout);
    printf("[Parent] Timeout set to %d seconds. Will kill children if they don't finish.\n", timeout);
  }

  // Ожидание завершения дочерних процессов с учетом таймаута
  if (timeout > 0) {
    int remaining = active_child_processes;
    while (remaining > 0) {
      int status;
      pid_t finished_pid = waitpid(-1, &status, WNOHANG);
      
      if (finished_pid > 0) {
        remaining--;
        if (WIFEXITED(status)) {
          printf("[Parent] Child %d finished normally with exit code %d\n", 
                 finished_pid, WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
          printf("[Parent] Child %d was terminated by signal %d\n", 
                 finished_pid, WTERMSIG(status));
        }
        for (int i = 0; i < children_count; i++) {
          if (child_pids[i] == finished_pid) {
            child_pids[i] = -1;
            break;
          }
        }
      } else if (finished_pid == -1 && errno != EINTR) {
        perror("waitpid");
        break;
      }
      
      usleep(100000);
    }
    
    alarm(0);
    
  } else {
    // Режим без таймаута - стандартное блокирующее ожидание
    printf("[Parent] No timeout. Waiting for all children to finish...\n");
    while (active_child_processes > 0) {
      wait(NULL);
      active_child_processes -= 1;
    }
  }

  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  for (int i = 0; i < pnum; i++) {
    int min = INT_MAX;
    int max = INT_MIN;
    bool child_finished = true;

    if (timeout > 0) {
      for (int j = 0; j < children_count; j++) {
        if (child_pids[j] == -1 && j == i) {
          break;
        } else if (child_pids[j] > 0 && j == i) {
          child_finished = false;
          printf("[Parent] Child %d did not produce results (killed by timeout)\n", child_pids[j]);
        }
      }
    }

    if (child_finished) {
      if (with_files) {
        char filename[30];
        sprintf(filename, "min_max_%d.txt", i);
        FILE *file = fopen(filename, "r");
        if (file != NULL) {
          fscanf(file, "%d %d", &min, &max);
          fclose(file);
          remove(filename);
        } else {
          printf("[Parent] Could not read results from child %d\n", i);
          continue;
        }
      } else {
        close(pipefd[2 * i + 1]);
        read(pipefd[2 * i], &min, sizeof(int));
        read(pipefd[2 * i], &max, sizeof(int));
        close(pipefd[2 * i]);
      }

      if (min < min_max.min) min_max.min = min;
      if (max > min_max.max) min_max.max = max;
    }
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);
  free(child_pids);

  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Elapsed time: %fms\n", elapsed_time);
  fflush(NULL);
  return 0;
}