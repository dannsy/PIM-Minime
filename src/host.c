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

#define SEQ_BINARY "./dpu/sequential_read"
#define RAND_BINARY "./dpu/random_read"

memory_bench_plugin_t plugins[] = {
    {"sequential_read"},
    {"random_read"},
};

unsigned nb_plugins = sizeof(plugins) / sizeof(memory_bench_plugin_t);

int chosen_plugin = -1;

#define DEFAULT_BENCH_TIME (10LL)                             // In seconds
#define DEFAULT_MEMORY_BENCH_SIZE_TO_BENCH (32 * 1024 * 1024) // In bytes

uint32_t per_dpu_memory_to_alloc = 0;

uint64_t bench_time = DEFAULT_BENCH_TIME;

// Each element of array is 4 bytes
static uint32_t input_buffer[BUFFER_SIZE];

void seq_init()
{
    for (int i = 0; i < BUFFER_SIZE; i++)
    {
        input_buffer[i] = (uint32_t)1;
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

    int index = 0;
    for (i = 1; i < BUFFER_SIZE; i++)
    {
        input_buffer[index] = rand_array[i].i;
        index = input_buffer[index];
    }
    input_buffer[index] = 0;

    free(rand_array);
}

void prepare_dpu()
{
    struct dpu_set_t dpu_set, dpu;

    DPU_ASSERT(dpu_alloc(1, NULL, &dpu_set));

    if (chosen_plugin == 0)
    {
        DPU_ASSERT(dpu_load(dpu_set, SEQ_BINARY, NULL));
        seq_init();
    }
    else
    {
        DPU_ASSERT(dpu_load(dpu_set, RAND_BINARY, NULL));
        rand_init();
    }

    DPU_ASSERT(dpu_copy_to(dpu_set, "buffer", 0, input_buffer, per_dpu_memory_to_alloc));
    DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));

    DPU_FOREACH(dpu_set, dpu)
    {
        DPU_ASSERT(dpu_log_read(dpu, stdout));
    }

    DPU_ASSERT(dpu_free(dpu_set));
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
    fprintf(stderr, "Usage: %s -t <plugin number> [ -l <size>[K|M]] [-T <time in seconds>]\n", app_name);
    fprintf(stderr, "\t-t: plugin number. Available plugins:\n");
    for (int i = 0; i < nb_plugins; i++)
    {
        printf("\t\t%d - %s\n", i, plugins[i].name);
    }

    fprintf(stderr, "\t-d: use DPU\n");
    fprintf(stderr, "\t-g: memory size to benchmark per DPU (Maximum 32MB)\n");
    fprintf(stderr, "\t-T: manually specify the benchmark duration (in seconds)\n");
    fprintf(stderr, "\t-h: display usage\n");
}

int main(int argc, char **argv)
{
    bool use_dpu = false;

    int opt;
    char const options[] = "hdt:g:T:";
    while ((opt = getopt(argc, argv, options)) != -1)
    {
        switch (opt)
        {
        case 'd':
            use_dpu = true;
            break;

        case 't':
            chosen_plugin = atoi(optarg);
            if (chosen_plugin < 0 || chosen_plugin >= nb_plugins)
            {
                fprintf(stderr, "%d is not a valid plugin number\n", chosen_plugin);
                return EXIT_FAILURE;
            }
            break;

        case 'g':
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
        fprintf(stderr, "Memory to allocate per DPU invalid, falling back to default memory size of 32MB\n");
        per_dpu_memory_to_alloc = DEFAULT_MEMORY_BENCH_SIZE_TO_BENCH;
    }

    printf("Bench parameters\n");
    printf("\t* Use DPU: %d\n", use_dpu);
    printf("\t* Chosen benchmark: %s\n", plugins[chosen_plugin].name);
    printf("\t* Memory size to bench per DPU: %u bytes\n", (unsigned)per_dpu_memory_to_alloc);
    printf("\t* Benchmark time: %lus\n", (unsigned long)bench_time);

    if (use_dpu)
    {
        prepare_dpu();
    }
    else
    {
        // TODO: add CPU only code here
    }

    return EXIT_SUCCESS;
}
