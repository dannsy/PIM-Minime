#ifndef COMMON_H_
#define COMMON_H_

/*
 * The maximum buffer size for each DPU is must be smaller than 64MB.
 * Each buffer element is 32 bits = 4 bytes, so if the buffer size is 32MB,
 * then the buffer needs 8 * 1024 * 1024 elements
 */
#define MAX_BUFFER_SIZE 8 * 1024 * 1024

/*
 * NR_DPUS and NR_TASKLETS definitions are here mainly for getting rid of "undefined" errors
 */
#ifndef NR_DPUS
#define NR_DPUS 1
#endif

#ifndef NR_TASKLETS
#define NR_TASKLETS 1
#endif

typedef struct
{
    const char *name;
} memory_bench_plugin_t;

/**
 * Contains the DPU inputs that the host has to copy into the DPUs
 * 
 * @var total_buffer_size - The total number of elements to read for all tasklets combined. Must be < MAX_BUFFER_SIZE
 * @var tasklet_buffer_size - The number of elements a tasklet has to read
 * @var tasklet_start_index - An array containing the starting index on the buffer for each tasklet
 * @var max_cycles - The maximum number of cycles for benchmark to run. Used to limit benchmark execution time
 */
typedef struct
{
    uint32_t total_buffer_size;
    uint32_t tasklet_buffer_size;
    uint32_t tasklet_start_index[NR_TASKLETS];
    uint64_t max_cycles;
} dpu_input_t;

/*
 * Contains the tasklet output
 * 
 * @var bytes_read - The total number of bytes read by a tasklet
 * @var cycles - The total number of cycles used to execute the benchmark for a tasklet
 */
typedef struct
{
    uint64_t bytes_read;
    uint64_t cycles;
} tasklet_output_t;

/*
 * Contains the DPU outputs that the host has to read out of the DPUs
 */
typedef struct
{
    tasklet_output_t tasklet_result[NR_TASKLETS];
} dpu_output_t;

#endif // COMMON_H_
