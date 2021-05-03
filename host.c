/**
 * @file host.c
 * @brief Host file to start microbenchmark on DPU sequential and random memory accesses
 * 
 */

#include <dpu.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include "common.h"

#define SEQ_BINARY "sequential_read"
#define RAND_BINARY "random_read"

int chosen_plugin = -1;

#define DEFAULT_BENCH_TIME (10LL)                             // In seconds
#define DEFAULT_MEMORY_BENCH_SIZE_TO_BENCH (32 * 1024 * 1024) // In bytes

uint64_t bench_time = DEFAULT_BENCH_TIME;

// Each element of array is 1 byte
static uint32_t input_buffer[BUFFER_SIZE];

void seq_init()
{
    for (int i = 0; i < BUFFER_SIZE; i++)
    {
        input_buffer[i] = (uint32_t)0;
    }
}

struct ij
{
    int i;
    int j;
};

/*
 * Compare function for qsort.
 * Sorts rand_array elements in increasing j order
 */
static int compar(const void *a1, const void *a2)
{
    struct ij *a, *b;
    a = (struct ij *)a1;
    b = (struct ij *)a2;
    return a->j - b->j;
}

void rand_init()
{
    int i;
    unsigned int seed = 1;

    struct ij *rand_array = malloc(sizeof(struct ij) * BUFFER_SIZE);

    for (i = 1; i < BUFFER_SIZE; i++)
    {
        rand_array[i].i = i;
        /* For all cores and their thread, generate the same sequence of random j */
        rand_array[i].j = rand_r(&seed);
    }
    /* Sort rand_array elements in increasing j order */
    qsort(&rand_array[1], BUFFER_SIZE - 1, sizeof(*rand_array), compar);

    // uint64_t *addr = (uint64_t *)memory_to_access;
    int index = 0;
    for (i = 1; i < BUFFER_SIZE; i++)
    {
        input_buffer[index] = rand_array[i].i;
        index = input_buffer[index];

        // *addr = (uint64_t)&memory_to_access[rand_array[i + 1].i];
        // addr = (uint64_t *)*addr;
    }
    input_buffer[index] = 0;

    free(rand_array);
}

void start_dpu()
{
}

static uint64_t parse_size(char *size)
{
    int length = strlen(size);
    int factor = 1;
    if (size[length - 1] == 'K')
    {
        factor = 1024;
        size[length - 1] = 0;
    }
    else if (size[length - 1] == 'M')
    {
        factor = 1024 * 1024;
        size[length - 1] = 0;
    }

    return (uint64_t)atoi(size) * factor;
}

static void usage(char *app_name)
{
    // unsigned long nb_plugins = sizeof(plugins) / sizeof(memory_bench_plugin_t);

    fprintf(stderr, "Usage: %s -t <plugin number> [ -l <size>[K|M]] [-T <time in seconds>]\n", app_name);
    fprintf(stderr, "\t-t: plugin number. Available plugins:\n");
    // int i = 0;
    // for (i = 0; i < nb_plugins; i++)
    // {
    //     printf("\t\t%d - %s\n", i, plugins[i].name);
    // }

    fprintf(stderr, "\t-d: use DPU\n");
    fprintf(stderr, "\t-l: memory size to benchmark per DPU (Maximum 64MB)\n");
    fprintf(stderr, "\t-T: manually specify the benchmark duration (in seconds)\n");
    fprintf(stderr, "\t-h: display usage\n");
}

int main(int argc, char **argv)
{
    struct dpu_set_t dpu_set, dpu;
    uint32_t per_dpu_memory_to_alloc = 0;
    bool use_dpu = false;

    int opt;
    char const options[] = "hdt:l:T:";
    while ((opt = getopt(argc, argv, options)) != -1)
    {
        switch (opt)
        {
        case 'd':
            use_dpu = true;
            break;

        case 't':
            chosen_plugin = atoi(optarg);
            // unsigned long nb_plugins = sizeof(plugins) / sizeof(memory_bench_plugin_t);
            unsigned long nb_plugins = 2;
            if (chosen_plugin < 0 || chosen_plugin >= nb_plugins)
            {
                fprintf(stderr, "%d is not a valid plugin number\n", chosen_plugin);
                return EXIT_FAILURE;
            }
            break;

        case 'l':
            per_dpu_memory_to_alloc = parse_size(optarg);
            break;

        case 'T':
            bench_time = atoll(optarg);
            break;

        case 'h':
            usage(argv[0]);
            return EXIT_SUCCESS;

        default:
            usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    if (chosen_plugin < 0)
    {
        fprintf(stderr, "Plugin was not chosen\n\n");
        usage(argv[0]);
        return EXIT_FAILURE;
    }
    else if (per_dpu_memory_to_alloc <= 0 || per_dpu_memory_to_alloc > DEFAULT_MEMORY_BENCH_SIZE_TO_BENCH)
    {
        fprintf(stderr, "Memory to allocate per DPU invalid, falling back to default memory size of 32MB");
        per_dpu_memory_to_alloc = DEFAULT_MEMORY_BENCH_SIZE_TO_BENCH;
    }

    printf("Bench parameters\n");
    // printf("\t* Required %d DPUs\n", NR_DPUS);
    // printf("\t* Required %d tasklets\n", NR_TASKLETS);
    printf("\t* Memory size to bench per DPU: %u bytes\n", (unsigned)per_dpu_memory_to_alloc);
    printf("\t* Benchmark time: %lus\n", (unsigned long)bench_time);

    DPU_ASSERT(dpu_alloc(1, NULL, &dpu_set));
    DPU_ASSERT(dpu_load(dpu_set, RAND_BINARY, NULL));

    rand_init();
    DPU_ASSERT(dpu_copy_to(dpu_set, "buffer", 0, input_buffer, per_dpu_memory_to_alloc));
    DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));

    DPU_FOREACH(dpu_set, dpu)
    {
        DPU_ASSERT(dpu_log_read(dpu, stdout));
    }

    DPU_ASSERT(dpu_free(dpu_set));

    return EXIT_SUCCESS;
}
