#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <getopt.h>

typedef struct {
    int start;
    int end;
    int mod;
    long long *result;
    pthread_mutex_t *mutex;
} ThreadData;

void* compute_partial(void* arg) {
    ThreadData *data = (ThreadData*) arg;
    long long partial = 1;

    for (int i = data->start; i <= data->end; i++) {
        partial = (partial * i) % data->mod;
    }

    pthread_mutex_lock(data->mutex);
    *data->result = (*data->result * partial) % data->mod;
    pthread_mutex_unlock(data->mutex);

    return NULL;
}

int main(int argc, char **argv) {
    int k = 0, pnum = 1, mod = 1;
    int opt;

    static struct option long_options[] = {
        {"k", required_argument, 0, 'k'},
        {"pnum", required_argument, 0, 'p'},
        {"mod", required_argument, 0, 'm'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "k:p:m:", long_options, NULL)) != -1) {
        switch (opt) {
            case 'k': k = atoi(optarg); break;
            case 'p': pnum = atoi(optarg); break;
            case 'm': mod = atoi(optarg); break;
            default: fprintf(stderr, "Usage: %s -k <num> --pnum=<num> --mod=<num>\n", argv[0]); exit(EXIT_FAILURE);
        }
    }

    if (k <= 0 || pnum <= 0 || mod <= 0) {
        fprintf(stderr, "All arguments must be positive integers.\n");
        exit(EXIT_FAILURE);
    }

    pthread_t threads[pnum];
    ThreadData thread_data[pnum];
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    long long total_result = 1;

    int chunk_size = k / pnum;
    int remainder = k % pnum;
    int current = 1;

    for (int i = 0; i < pnum; i++) {
        int end = current + chunk_size - 1 + (i < remainder ? 1 : 0);
        if (end > k) end = k;

        thread_data[i].start = current;
        thread_data[i].end = end;
        thread_data[i].mod = mod;
        thread_data[i].result = &total_result;
        thread_data[i].mutex = &mutex;

        pthread_create(&threads[i], NULL, compute_partial, &thread_data[i]);

        current = end + 1;
    }

    for (int i = 0; i < pnum; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Factorial %d! mod %d = %lld\n", k, mod, total_result);
    return 0;
}