#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>

#include "common.h"

struct Server {
  char ip[255];
  int port;
};

struct ThreadArgs {
  struct Server server;
  uint64_t begin;
  uint64_t end;
  uint64_t mod;
  uint64_t result;
  int status;
};

bool ConvertStringToUI64(const char *str, uint64_t *val) {
  char *end = NULL;
  unsigned long long i = strtoull(str, &end, 10);
  if (errno == ERANGE) {
    fprintf(stderr, "Out of uint64_t range: %s\n", str);
    return false;
  }

  if (errno != 0)
    return false;

  *val = i;
  return true;
}

void *WorkerThread(void *arg) {
  struct ThreadArgs *args = (struct ThreadArgs *)arg;
  
  struct hostent *hostname = gethostbyname(args->server.ip);
  if (hostname == NULL) {
    fprintf(stderr, "gethostbyname failed with %s\n", args->server.ip);
    args->status = -1;
    return NULL;
  }

  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_port = htons(args->server.port);
  server.sin_addr.s_addr = *((unsigned long *)hostname->h_addr);

  int sck = socket(AF_INET, SOCK_STREAM, 0);
  if (sck < 0) {
    fprintf(stderr, "Socket creation failed!\n");
    args->status = -1;
    return NULL;
  }

  if (connect(sck, (struct sockaddr *)&server, sizeof(server)) < 0) {
    fprintf(stderr, "Connection failed to %s:%d\n", args->server.ip, args->server.port);
    args->status = -1;
    close(sck);
    return NULL;
  }

  char task[sizeof(uint64_t) * 3];
  memcpy(task, &args->begin, sizeof(uint64_t));
  memcpy(task + sizeof(uint64_t), &args->end, sizeof(uint64_t));
  memcpy(task + 2 * sizeof(uint64_t), &args->mod, sizeof(uint64_t));

  if (send(sck, task, sizeof(task), 0) < 0) {
    fprintf(stderr, "Send failed\n");
    args->status = -1;
    close(sck);
    return NULL;
  }

  char response[sizeof(uint64_t)];
  if (recv(sck, response, sizeof(response), 0) < 0) {
    fprintf(stderr, "Receive failed\n");
    args->status = -1;
    close(sck);
    return NULL;
  }

  memcpy(&args->result, response, sizeof(uint64_t));
  args->status = 0;
  close(sck);
  return NULL;
}

int main(int argc, char **argv) {
  uint64_t k = -1;
  uint64_t mod = -1;
  char servers[255] = {'\0'};

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"k", required_argument, 0, 0},
                                      {"mod", required_argument, 0, 0},
                                      {"servers", required_argument, 0, 0},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);

    if (c == -1)
      break;

    switch (c) {
    case 0: {
      switch (option_index) {
      case 0:
        ConvertStringToUI64(optarg, &k);
        break;
      case 1:
        ConvertStringToUI64(optarg, &mod);
        break;
      case 2:
        memcpy(servers, optarg, strlen(optarg));
        break;
      default:
        printf("Index %d is out of options\n", option_index);
      }
    } break;

    case '?':
      printf("Arguments error\n");
      break;
    default:
      fprintf(stderr, "getopt returned character code 0%o?\n", c);
    }
  }

  if (k == -1 || mod == -1 || !strlen(servers)) {
    fprintf(stderr, "Using: %s --k 1000 --mod 5 --servers /path/to/file\n",
            argv[0]);
    return 1;
  }

  FILE *file = fopen(servers, "r");
  if (!file) {
    fprintf(stderr, "Failed to open servers file: %s\n", servers);
    return 1;
  }

  struct Server *to = NULL;
  unsigned int servers_num = 0;
  char line[512];
  
  while (fgets(line, sizeof(line), file)) {
    if (line[0] == '\n') continue;
    to = realloc(to, sizeof(struct Server) * (servers_num + 1));
    char ip[255];
    int port;
    if (sscanf(line, "%255[^:]:%d", ip, &port) == 2) {
      strcpy(to[servers_num].ip, ip);
      to[servers_num].port = port;
      servers_num++;
    }
  }
  fclose(file);

  if (servers_num == 0) {
    fprintf(stderr, "No servers found in file\n");
    free(to);
    return 1;
  }

  struct ThreadArgs *args = malloc(sizeof(struct ThreadArgs) * servers_num);
  pthread_t *threads = malloc(sizeof(pthread_t) * servers_num);
  
  uint64_t chunk_size = k / servers_num;
  uint64_t remainder = k % servers_num;
  uint64_t current_begin = 1;
  
  for (unsigned int i = 0; i < servers_num; i++) {
    args[i].server = to[i];
    args[i].begin = current_begin;
    args[i].end = current_begin + chunk_size - 1;
    if (i < remainder) {
      args[i].end++;
    }
    args[i].mod = mod;
    current_begin = args[i].end + 1;
    
    pthread_create(&threads[i], NULL, WorkerThread, &args[i]);
  }
  
  uint64_t total = 1;
  for (unsigned int i = 0; i < servers_num; i++) {
    pthread_join(threads[i], NULL);
    if (args[i].status == 0) {
      total = MultModulo(total, args[i].result, mod);
    } else {
      fprintf(stderr, "Server %s:%d failed\n", args[i].server.ip, args[i].server.port);
    }
  }
  
  printf("answer: %lu\n", total);
  
  free(args);
  free(threads);
  free(to);

  return 0;
}