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

// MACROS containing path to DPU executables
#define SEQ_BINARY "./dpu/sequential_read"
#define RAND_BINARY "./dpu/random_read"
#define TRANSFER_BINARY "./dpu/wram_transfer"

// MACROS for helping with getting the execution time
#define TIME_NOW(_t) (clock_gettime(CLOCK_MONOTONIC, (_t)))
#define TIME_DIFFERENCE(_start, _end)       \
    ((_end.tv_sec + _end.tv_nsec / 1.0e9) - \
     (_start.tv_sec + _start.tv_nsec / 1.0e9))

#define DEFAULT_BENCH_TIME (10LL)                             // In seconds
#define DEFAULT_MEMORY_BENCH_SIZE_TO_BENCH (32 * 1024 * 1024) // In bytes

// Benchmark options constants
memory_bench_plugin_t plugins[] = {
    {"wram_transfer"},
    {"sequential_read"},
    {"random_read"},
};
unsigned nb_plugins = sizeof(plugins) / sizeof(memory_bench_plugin_t);

// Buffer to copy into DPUs, each element is 4 bytes
static uint8_t input_buffer[MAX_BUFFER_SIZE];

/**
 * DPU runtime struct containing execution time for various operations
 */
typedef struct dpu_runtime
{
    float copy_in_time;
    float benchmark_time;
    float copy_out_time;
} dpu_runtime;

/**
 * Initialize the array for the sequential read plugin
 * Fill in all array elements with 0
 * 
 * @param[in] buffer_size - The number of elements to fill in for the array
 */
void seq_init(uint32_t buffer_size)
{
    for (int i = 0; i < buffer_size; i++)
    {
        input_buffer[i] = (uint8_t)0;
    }
}

/**
 * Helper struct for randomly sorting the array for random read
 */
struct ij
{
    int i;
    int j;
};

/*
 * Compare function for qsort
 * Sorts rand_array elements in increasing j order
 */
static int compar(const void *a1, const void *a2)
{
    struct ij *a, *b;
    a = (struct ij *)a1;
    b = (struct ij *)a2;
    return a->j - b->j;
}

/**
 * Initialize the array for the random read plugin
 * Fill in buffer with pointer chasing elements
 * Each element should only be visited once for all tasklets combined
 * 
 * @param[in] buffer_size - The number of elements to fill in for the array
 * @param[in] dpu_input - Inputs that will be copied to the DPUs. Fill in start_index array during pointer chasing
 */
void rand_init(uint32_t buffer_size, dpu_input_t *dpu_input)
{
    buffer_size /= 2;
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
        input_buffer[index] = rand_array[i].i * 2;
        index = input_buffer[index];

        if ((i * 2) % dpu_input->tasklet_buffer_size == 0 && j < NR_TASKLETS)
        {
            dpu_input->tasklet_start_index[j] = index;
            j++;
        }
    }
    input_buffer[index] = 0;

    free(rand_array);
}

/**
 * Prepare, start, and collect results from DPUs
 * 
 * @param[in] chosen_plugin - Which plugin the user chose. Either 0 for sequential_read or 1 for random_read
 * @param[in] per_dpu_memory_to_alloc - The number of bytes to copy from input_buffer to DPU MRAM
 * @param[in] buffer_size - The number of elements to initialize in input_buffer
 * @param[in] bench_time - The number of seconds the benchmark should run for
 * @param[in] rt - Execution time struct to record how long each operation of the DPU took
 */
uint64_t start_dpu(int chosen_plugin, uint32_t per_dpu_memory_to_alloc, uint32_t buffer_size, uint64_t bench_time, dpu_runtime *rt)
{
    struct dpu_set_t dpu_set, dpu;
    struct timespec start, end;

    DPU_ASSERT(dpu_alloc(NR_DPUS, NULL, &dpu_set));

    dpu_input_t dpu_input;
    dpu_input.total_buffer_size = buffer_size;
    dpu_input.tasklet_buffer_size = buffer_size / NR_TASKLETS;
    dpu_input.bench_time = bench_time;
    for (int i = 0; i < NR_TASKLETS; i++)
    {
        dpu_input.tasklet_start_index[i] = i * dpu_input.tasklet_buffer_size;
    }

    printf("Buffer size: %u\n", dpu_input.total_buffer_size);
    printf("Tasklet buffer size: %u\n", dpu_input.tasklet_buffer_size);

    // Load the DPU binary executable and initialize the buffer depending on chosen plugin
    switch (chosen_plugin)
    {
    case (0):
        // mram transfer
        DPU_ASSERT(dpu_load(dpu_set, TRANSFER_BINARY, NULL));
        seq_init(buffer_size);
        break;
    case (1):
        // sequential read
        DPU_ASSERT(dpu_load(dpu_set, SEQ_BINARY, NULL));
        seq_init(buffer_size);
        break;
    case (2):
        // random read
        DPU_ASSERT(dpu_load(dpu_set, RAND_BINARY, NULL));
        rand_init(buffer_size, &dpu_input);
        break;
    default:
        fprintf(stderr, "Unexpected error\n");
        return EXIT_FAILURE;
    }

    // Copy inputs to the DPUs
    TIME_NOW(&start);
    DPU_ASSERT(dpu_copy_to(dpu_set, "buffer", 0, input_buffer, per_dpu_memory_to_alloc));
    DPU_ASSERT(dpu_copy_to(dpu_set, "input", 0, &dpu_input, sizeof(dpu_input_t)));
    TIME_NOW(&end);

    rt->copy_in_time = TIME_DIFFERENCE(start, end);

    // Start the benchmark
    TIME_NOW(&start);
    DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    TIME_NOW(&end);

    rt->benchmark_time = TIME_DIFFERENCE(start, end);

    DPU_FOREACH(dpu_set, dpu)
    {
        DPU_ASSERT(dpu_log_read(dpu, stdout));
    }

    dpu_output_t results[NR_DPUS];
    uint32_t each_dpu;

    // Copy results from the DPUs
    TIME_NOW(&start);
    DPU_FOREACH(dpu_set, dpu, each_dpu)
    {
        DPU_ASSERT(dpu_prepare_xfer(dpu, &results[each_dpu]));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, "results", 0, sizeof(dpu_output_t), DPU_XFER_DEFAULT));
    TIME_NOW(&end);

    rt->copy_out_time = TIME_DIFFERENCE(start, end);

    uint64_t total_bytes_read = 0;
    DPU_FOREACH(dpu_set, dpu, each_dpu)
    {
        uint64_t bytes_read = 0, cycles = 0;
        // retrieve tasklet results
        for (unsigned int each_tasklet = 0; each_tasklet < NR_TASKLETS; each_tasklet++)
        {
            tasklet_output_t *result = &results[each_dpu].tasklet_result[each_tasklet];

            bytes_read += result->bytes_read;
            cycles += result->cycles;

            if (chosen_plugin == 0)
            {
                printf("Transfer size: %u, mram_read cycles: %lu, mram_write cycles: %lu\n",
                       result->transfer_size, result->read_cycles, result->write_cycles);
            }
        }

        cycles /= NR_TASKLETS;

        // float mbytes_read = bytes_read / 1024. / 1024.;
        // printf("\nBytes read: %lu bytes (%.2f MB)\n", bytes_read, mbytes_read);
        printf("Average cycles per DPU: %lu cycles\n", cycles);
        // printf("Time taken to run benchmark: %f seconds\n", time_diff);
        // printf("Throughput: %.2f MB/s\n", mbytes_read / time_diff);

        total_bytes_read += bytes_read;
    }

    DPU_ASSERT(dpu_free(dpu_set));

    return total_bytes_read;
}

/**
 * Helper function to parse user input memory size
 */
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

/**
 * Helper function to display the proper usage of this program
 */
static void usage(char *app_name)
{
    fprintf(stderr, "Usage: %s -t <plugin number> [ -l <size>[K|M]] [-T <time in seconds>]\n", app_name);
    fprintf(stderr, "\t-t: plugin number. Available plugins:\n");
    for (int i = 0; i < nb_plugins; i++)
    {
        printf("\t\t%d - %s\n", i, plugins[i].name);
    }

    fprintf(stderr, "\t-g: memory size to benchmark per DPU (Maximum 32MB)\n");
    fprintf(stderr, "\t-T: manually specify the benchmark duration (in seconds)\n");
    fprintf(stderr, "\t-h: display usage\n");
}

int main(int argc, char **argv)
{
    int chosen_plugin = -1;

    uint32_t per_dpu_memory_to_alloc = 0;
    uint32_t buffer_size = 0;
    uint64_t bench_time = DEFAULT_BENCH_TIME;

    int opt;
    char const options[] = "ht:g:T:";
    while ((opt = getopt(argc, argv, options)) != -1)
    {
        switch (opt)
        {
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
    buffer_size = per_dpu_memory_to_alloc;

    printf("Bench parameters\n");
    printf("\t* Num DPUs: %d\n", NR_DPUS);
    printf("\t* Num tasklets: %d\n", NR_TASKLETS);
    printf("\t* Chosen benchmark: %s\n", plugins[chosen_plugin].name);
    printf("\t* Buffer size to bench per DPU: %u bytes (%.2f MB)\n", per_dpu_memory_to_alloc, per_dpu_memory_to_alloc / 1024. / 1024.);
    printf("\t* Benchmark time: %lus\n", (unsigned long)bench_time);

    dpu_runtime runtime;
    uint64_t total_bytes_read = start_dpu(chosen_plugin, per_dpu_memory_to_alloc, buffer_size, bench_time, &runtime);

    float total_mbytes_read = total_bytes_read / 1024. / 1024.;
    printf("Bench results\n");
    printf("\t* Copy in took: %fs\n", runtime.copy_in_time);
    printf("\t* Benchmarking took: %fs\n", runtime.benchmark_time);
    printf("\t* Copy out took: %fs\n", runtime.copy_out_time);

    printf("\t* Total bytes read: %lu bytes (%.2f MB)\n", total_bytes_read, total_mbytes_read);
    printf("\t* Overall throughput: %.2f MB/s\n", total_mbytes_read / runtime.benchmark_time);

    return EXIT_SUCCESS;
}
