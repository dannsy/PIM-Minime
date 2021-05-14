#include <stdio.h>
#include <perfcounter.h>
#include <mram.h>
#include <defs.h>

#include "common.h"

#ifndef BLOCK_SIZE
#define BLOCK_SIZE 8
#endif
#define UNROLL_SIZE 1

__mram_noinit uint32_t buffer[8388608];
__host dpu_input_t input;
__host dpu_output_t results;

int main()
{
    __dma_aligned uint32_t cache[BLOCK_SIZE];
    uint32_t tasklet_id = me();
    tasklet_output_t *result = &results.tasklet_result[tasklet_id];

    uint32_t total_buffer_size = input.total_buffer_size;
    // This will only be *entirely accurate* if NR_TASKLETS is a power of 2
    uint32_t buffer_size_per_tasket = ((input.tasklet_buffer_size / UNROLL_SIZE) * UNROLL_SIZE) >> 2;
    uint64_t max_cycles = input.bench_time * CLOCKS_PER_SEC;

    if (tasklet_id == 0)
    {
        perfcounter_config(COUNT_CYCLES, true);
    }

    uint32_t nb_iterations = 0;
    uint32_t transfer_size = BLOCK_SIZE;
    uint64_t cycles;
    while (1)
    {
        uint32_t buffer_i = 0;
        uint32_t index = input.tasklet_start_index[tasklet_id];
        // unroll loop to have greater ratio of load instructions to branch instructions
        for (; buffer_i < buffer_size_per_tasket; buffer_i++)
        {
            // pointer chasing random read
            mram_read(&buffer[index], cache, transfer_size);
            index = cache[0];
        }
        nb_iterations++;

        cycles = perfcounter_get();
        if (cycles > max_cycles)
        {
            break;
        }
    }

    result->cycles = perfcounter_get();
    result->bytes_read = nb_iterations * buffer_size_per_tasket * transfer_size;
    return 0;
}
