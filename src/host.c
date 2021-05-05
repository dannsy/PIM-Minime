/**
 * @file host.c
 * @brief Host file to start microbenchmark on DPU sequential and random memory accesses
 * 
 */

#include <dpu.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <time.h>

#include "common.h"

#define SEQ_BINARY "./dpu/sequential_read"
#define RAND_BINARY "./dpu/random_read"

#define TIME_NOW(_t) (clock_gettime(CLOCK_MONOTONIC, (_t)))
#define TIME_DIFFERENCE(_start, _end)       \
    ((_end.tv_sec + _end.tv_nsec / 1.0e9) - \
     (_start.tv_sec + _start.tv_nsec / 1.0e9))

memory_bench_plugin_t plugins[] = {
    {"sequential_read"},
    {"random_read"},
};

unsigned nb_plugins = sizeof(plugins) / sizeof(memory_bench_plugin_t);

int chosen_plugin = -1;

#define DEFAULT_BENCH_TIME (10LL)                             // In seconds
#define DEFAULT_MEMORY_BENCH_SIZE_TO_BENCH (32 * 1024 * 1024) // In bytes

uint32_t per_dpu_memory_to_alloc = 0;
uint32_t buffer_size = 0;

uint64_t bench_time = DEFAULT_BENCH_TIME;

// Each element of array is 4 bytes
static uint32_t input_buffer[MAX_BUFFER_SIZE];

void seq_init()
{
    for (int i = 0; i < buffer_size; i++)
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

void rand_init(dpu_input_t *dpu_input)
{
    int i;
    unsigned int seed = 1;

    struct ij *rand_array = malloc(sizeof(struct ij) * buffer_size);

    for (i = 1; i < buffer_size; i++)
    {
        rand_array[i].i = i;
        /* For all cores and their thread, generate the same sequence of random j */
        rand_array[i].j = rand_r(&seed);
    }
    /* Sort rand_array elements in increasing j order */
    qsort(&rand_array[1], buffer_size - 1, sizeof(*rand_array), compar);

    int index = 0;
    int j = 1;
    for (i = 1; i < buffer_size; i++)
    {
        input_buffer[index] = rand_array[i].i;
        index = input_buffer[index];

        if (i % dpu_input->tasklet_buffer_size == 0)
        {
            dpu_input->tasklet_start_index[j] = index;
            j++;
        }
    }
    input_buffer[index] = 0;

    free(rand_array);
}

// TODO: Calculate time spent doing benchmarking
// TODO: Let benchmark run until time limit reached instead of only doing one loop
void prepare_dpu()
{
    struct dpu_set_t dpu_set, dpu;
    struct timespec start, end;

    DPU_ASSERT(dpu_alloc(NR_DPUS, NULL, &dpu_set));

    dpu_input_t dpu_input;
    dpu_input.total_buffer_size = buffer_size;
    dpu_input.tasklet_buffer_size = buffer_size / NR_TASKLETS;
    dpu_input.max_cycles = (uint64_t)0;
    for (int i = 0; i < NR_TASKLETS; i++)
    {
        dpu_input.tasklet_start_index[i] = i * dpu_input.tasklet_buffer_size;
    }

    if (chosen_plugin == 0)
    {
        DPU_ASSERT(dpu_load(dpu_set, SEQ_BINARY, NULL));
        seq_init();
    }
    else
    {
        DPU_ASSERT(dpu_load(dpu_set, RAND_BINARY, NULL));
        rand_init(&dpu_input);
    }

    DPU_ASSERT(dpu_copy_to(dpu_set, "buffer", 0, input_buffer, per_dpu_memory_to_alloc));
    DPU_ASSERT(dpu_copy_to(dpu_set, "input", 0, &dpu_input, sizeof(dpu_input_t)));

    TIME_NOW(&start);
    DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    TIME_NOW(&end);

    // DPU_FOREACH(dpu_set, dpu)
    // {
    //     DPU_ASSERT(dpu_log_read(dpu, stdout));
    // }

    dpu_output_t results[NR_DPUS];
    uint32_t each_dpu;
    uint64_t bytes_read = 0, cycles = 0;
    DPU_FOREACH(dpu_set, dpu, each_dpu)
    {
        DPU_ASSERT(dpu_prepare_xfer(dpu, &results[each_dpu]));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, "results", 0, sizeof(dpu_output_t), DPU_XFER_DEFAULT));

    DPU_FOREACH(dpu_set, dpu, each_dpu)
    {
        // retrieve tasklet results
        for (unsigned int each_tasklet = 0; each_tasklet < NR_TASKLETS; each_tasklet++)
        {
            tasklet_output_t *result = &results[each_dpu].tasklet_result[each_tasklet];

            bytes_read += result->bytes_read;
            cycles += result->cycles;
        }

        cycles /= NR_TASKLETS;

        printf("Total bytes read: %lu bytes\n", bytes_read);
        printf("Average cycles per DPU: %lu cycles\n", cycles);
        printf("Time taken to run benchmark: %f seconds\n", TIME_DIFFERENCE(start, end));
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
    buffer_size = per_dpu_memory_to_alloc / sizeof(uint32_t);

    printf("Bench parameters\n");
    printf("\t* Use DPU: %d\n", use_dpu);
    printf("\t* Chosen benchmark: %s\n", plugins[chosen_plugin].name);
    printf("\t* Memory size to bench per DPU: %u bytes\n", per_dpu_memory_to_alloc);
    printf("\t* Buffer size: %u\n", buffer_size);
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
