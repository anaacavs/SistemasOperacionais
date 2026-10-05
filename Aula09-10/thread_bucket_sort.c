#define _POSIX_C_SOURCE 200809L

#include "bucket_sort_common.h"
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

typedef struct {
    const int *input;
    size_t count;
    int minimum;
    uint64_t range;
    size_t first_bucket;
    size_t end_bucket;
    Node **buckets;
    int failed;
} Worker;

static int arrays_equal(const int *left, const int *right, size_t count)
{
    for (size_t i = 0; i < count; ++i)
        if (left[i] != right[i])
            return 0;
    return 1;
}

static void *sort_bucket_range(void *argument)
{
    Worker *worker = argument;
    for (size_t i = 0; i < worker->count; ++i) {
        size_t index = bucket_index(worker->input[i], worker->minimum,
                                    worker->range, worker->count);
        if (index >= worker->first_bucket && index < worker->end_bucket &&
            insert_sorted(&worker->buckets[index - worker->first_bucket],
                          worker->input[i]) != 0) {
            worker->failed = 1;
            break;
        }
    }
    return NULL;
}

static int parse_thread_count(const char *text, size_t *thread_count)
{
    if (text && strcmp(text, "max") == 0) {
        long online = sysconf(_SC_NPROCESSORS_ONLN);
        if (online < 1) {
            fprintf(stderr, "Nao foi possivel obter o numero de CPUs online.\n");
            return -1;
        }
        *thread_count = (size_t)online;
        return 0;
    }

    if (!text || !*text || *text == '-')
        return -1;
    errno = 0;
    char *end = NULL;
    unsigned long parsed = strtoul(text, &end, 10);
    if (errno || *end != '\0' || parsed == 0 || parsed > SIZE_MAX)
        return -1;
    *thread_count = (size_t)parsed;
    return 0;
}

int main(int argc, char **argv)
{
    if (argc > 3) {
        fprintf(stderr, "Uso: %s [arquivo_entrada] [threads|max]\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *path = argc >= 2 ? argv[1] : "entradas/pequena.txt";
    const char *thread_arg = argc == 3 ? argv[2] : "max";
    size_t thread_count = 0, count = 0;
    if (parse_thread_count(thread_arg, &thread_count) != 0) {
        fprintf(stderr, "Numero de threads invalido. Use um inteiro positivo ou max.\n");
        return EXIT_FAILURE;
    }

    int *values = read_input(path, &count);
    if (count == SIZE_MAX)
        return EXIT_FAILURE;

    int *expected = count ? malloc(count * sizeof(*expected)) : NULL;
    if (count && !expected) {
        perror("malloc");
        free(values);
        return EXIT_FAILURE;
    }
    if (count)
        memcpy(expected, values, count * sizeof(*values));
    if (bucket_sort(expected, count) != 0) {
        free(expected);
        free(values);
        return EXIT_FAILURE;
    }

    pthread_t *threads = calloc(thread_count, sizeof(*threads));
    Worker *workers = calloc(thread_count, sizeof(*workers));
    if (!threads || !workers) {
        perror("calloc");
        free(threads);
        free(workers);
        free(expected);
        free(values);
        return EXIT_FAILURE;
    }

    struct timespec start, end;
    if (clock_gettime(CLOCK_MONOTONIC, &start) != 0) {
        perror("clock_gettime");
        free(threads); free(workers); free(expected); free(values);
        return EXIT_FAILURE;
    }

    int minimum = 0, maximum = 0;
    if (count) {
        minimum = maximum = values[0];
        for (size_t i = 1; i < count; ++i) {
            if (values[i] < minimum) minimum = values[i];
            if (values[i] > maximum) maximum = values[i];
        }
    }
    uint64_t range = count ? (uint64_t)((int64_t)maximum - (int64_t)minimum) + 1 : 1;

    size_t created = 0;
    int failed = 0;
    for (size_t i = 0; i < thread_count; ++i) {
        Worker *worker = &workers[i];
        worker->input = values;
        worker->count = count;
        worker->minimum = minimum;
        worker->range = range;
        size_t base = count / thread_count;
        size_t remainder = count % thread_count;
        worker->first_bucket = base * i + (i < remainder ? i : remainder);
        worker->end_bucket = worker->first_bucket + base + (i < remainder ? 1 : 0);
        size_t local_count = worker->end_bucket - worker->first_bucket;
        worker->buckets = local_count ? calloc(local_count, sizeof(*worker->buckets)) : NULL;
        if (local_count && !worker->buckets) {
            perror("calloc");
            failed = 1;
            break;
        }
        int error = pthread_create(&threads[i], NULL, sort_bucket_range, worker);
        if (error != 0) {
            fprintf(stderr, "pthread_create: %s\n", strerror(error));
            failed = 1;
            break;
        }
        ++created;
    }

    for (size_t i = 0; i < created; ++i) {
        int error = pthread_join(threads[i], NULL);
        if (error != 0) {
            fprintf(stderr, "pthread_join: %s\n", strerror(error));
            failed = 1;
        }
        if (workers[i].failed)
            failed = 1;
    }

    size_t output = 0;
    if (!failed && created == thread_count) {
        for (size_t i = 0; i < thread_count; ++i) {
            size_t local_count = workers[i].end_bucket - workers[i].first_bucket;
            for (size_t j = 0; j < local_count; ++j) {
                Node *node = workers[i].buckets[j];
                while (node) {
                    values[output++] = node->value;
                    Node *next = node->next;
                    free(node);
                    node = next;
                }
                workers[i].buckets[j] = NULL;
            }
        }
        if (output != count) {
            fprintf(stderr, "Erro: quantidade de valores na saida incorreta.\n");
            failed = 1;
        }
    }

    if (clock_gettime(CLOCK_MONOTONIC, &end) != 0) {
        perror("clock_gettime");
        failed = 1;
    }
    if (!failed && !arrays_equal(values, expected, count)) {
        fprintf(stderr, "Erro: resultado paralelo difere do sequencial.\n");
        failed = 1;
    }
    for (size_t i = 0; i < thread_count; ++i)
        free_buckets(workers[i].buckets,
                     workers[i].end_bucket - workers[i].first_bucket);
    free(workers);
    free(threads);

    if (failed) {
        free(expected); free(values);
        return EXIT_FAILURE;
    }

    double seconds = (double)(end.tv_sec - start.tv_sec) +
                     (double)(end.tv_nsec - start.tv_nsec) / 1e9;
    printf("Threads: %zu\nVerificacao sequencial/paralela: OK\n", thread_count);
    print_result(values, count, seconds);
    free(expected);
    free(values);
    return EXIT_SUCCESS;
}
