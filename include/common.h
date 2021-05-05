#ifndef COMMON_H_
#define COMMON_H_

/*
 * The maximum buffer size for each DPU is must be smaller than 64MB.
 * Each buffer element is 32 bits = 4 bytes, so if the buffer size is 32MB,
 * then the buffer needs 8 * 1024 * 1024 elements
 */
#define MAX_BUFFER_SIZE 8 * 1024 * 1024

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

/*
 * Contains the DPU inputs that the host has to copy into the DPUs
 */
typedef struct
{
    uint32_t buffer_size;
} dpu_input_t;

/*
 * Contains the tasklet output
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
