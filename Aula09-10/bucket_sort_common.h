#ifndef BUCKET_SORT_COMMON_H
#define BUCKET_SORT_COMMON_H

#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

typedef struct Node {
    int value;
    struct Node *next;
} Node;

static int *read_input(const char *path, size_t *length)
{
    *length = SIZE_MAX;
    FILE *file = fopen(path, "r");
    if (!file) {
        perror(path);
        return NULL;
    }

    int count;
    if (fscanf(file, "%d", &count) != 1 || count < 0) {
        fprintf(stderr, "Entrada invalida: a primeira linha deve conter N >= 0.\n");
        fclose(file);
        return NULL;
    }

    int *values = count ? malloc((size_t)count * sizeof(*values)) : NULL;
    if (count && !values) {
        perror("malloc");
        fclose(file);
        return NULL;
    }

    for (int i = 0; i < count; ++i) {
        if (fscanf(file, "%d", &values[i]) != 1) {
            fprintf(stderr, "Entrada invalida: faltou o valor %d de %d.\n", i + 1, count);
            free(values);
            fclose(file);
            return NULL;
        }
    }

    int character;
    while ((character = fgetc(file)) != EOF) {
        if (!isspace((unsigned char)character)) {
            fprintf(stderr, "Entrada invalida: ha conteudo alem dos %d valores declarados.\n", count);
            free(values);
            fclose(file);
            return NULL;
        }
    }

    fclose(file);
    *length = (size_t)count;
    return values;
}

static size_t bucket_index(int value, int minimum, uint64_t range, size_t count)
{
    uint64_t offset = (uint64_t)((int64_t)value - (int64_t)minimum);
    return (size_t)(offset * (uint64_t)count / range);
}

static int insert_sorted(Node **head, int value)
{
    Node **position = head;
    while (*position && (*position)->value <= value)
        position = &(*position)->next;

    Node *node = malloc(sizeof(*node));
    if (!node) {
        perror("malloc");
        return -1;
    }
    node->value = value;
    node->next = *position;
    *position = node;
    return 0;
}

static void free_buckets(Node **buckets, size_t count)
{
    if (!buckets)
        return;
    for (size_t i = 0; i < count; ++i) {
        Node *node = buckets[i];
        while (node) {
            Node *next = node->next;
            free(node);
            node = next;
        }
    }
    free(buckets);
}

static int bucket_sort(int *values, size_t count)
{
    if (count < 2)
        return 0;

    int minimum = values[0], maximum = values[0];
    for (size_t i = 1; i < count; ++i) {
        if (values[i] < minimum) minimum = values[i];
        if (values[i] > maximum) maximum = values[i];
    }
    uint64_t range = (uint64_t)((int64_t)maximum - (int64_t)minimum) + 1;
    Node **buckets = calloc(count, sizeof(*buckets));
    if (!buckets) {
        perror("calloc");
        return -1;
    }

    for (size_t i = 0; i < count; ++i) {
        size_t index = bucket_index(values[i], minimum, range, count);
        if (insert_sorted(&buckets[index], values[i]) != 0) {
            free_buckets(buckets, count);
            return -1;
        }
    }

    size_t output = 0;
    for (size_t i = 0; i < count; ++i) {
        Node *node = buckets[i];
        while (node) {
            values[output++] = node->value;
            Node *next = node->next;
            free(node);
            node = next;
        }
    }
    free(buckets);
    return 0;
}

static void print_result(const int *values, size_t count, double seconds)
{
    puts("Array ordenado:");
    for (size_t i = 0; i < count; ++i)
        printf("%d\n", values[i]);
    printf("Tempo de ordenacao: %.6f segundos\n", seconds);
}

#endif
