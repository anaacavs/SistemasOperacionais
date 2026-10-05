#define _POSIX_C_SOURCE 200809L

#include "bucket_sort_common.h"
#include <time.h>

int main(int argc, char **argv)
{
    if (argc > 2) {
        fprintf(stderr, "Uso: %s [arquivo_entrada]\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *path = argc == 2 ? argv[1] : "entradas/pequena.txt";
    size_t count = 0;
    int *values = read_input(path, &count);
    if (count == SIZE_MAX)
        return EXIT_FAILURE;

    struct timespec start, end;
    if (clock_gettime(CLOCK_MONOTONIC, &start) != 0 || bucket_sort(values, count) != 0 ||
        clock_gettime(CLOCK_MONOTONIC, &end) != 0) {
        free(values);
        return EXIT_FAILURE;
    }

    double seconds = (double)(end.tv_sec - start.tv_sec) +
                     (double)(end.tv_nsec - start.tv_nsec) / 1e9;
    print_result(values, count, seconds);
    free(values);
    return EXIT_SUCCESS;
}
