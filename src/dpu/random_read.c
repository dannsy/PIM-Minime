#include <stdio.h>
#include <perfcounter.h>
#include <mram.h>
#include <defs.h>
#include <barrier.h>

#include "common.h"

#define UNROLL_SIZE 8
#define CLOCKS_PER_SEC

__mram_noinit uint32_t buffer[MAX_BUFFER_SIZE];
__host dpu_input_t input;
__host dpu_output_t results;

int main()
{
    uint32_t tasklet_id = me();
    tasklet_output_t *result = &results.tasklet_result[tasklet_id];

    uint32_t total_buffer_size = input.total_buffer_size;
    // This will only be *entirely accurate* if NR_TASKLETS is a power of 2
    uint32_t buffer_size_per_tasket = (input.tasklet_buffer_size / UNROLL_SIZE) * UNROLL_SIZE;
    uint64_t max_cycles = input.max_cycles;

    if (tasklet_id == 0)
    {
        perfcounter_config(COUNT_CYCLES, true);
    }

    uint32_t nb_iterations = 0;
    uint64_t cycles;
    while (1)
    {
        uint32_t buffer_i = 0;
        uint32_t index = input.tasklet_start_index[tasklet_id];
        // unroll loop to have greater ratio of load instructions to branch instructions
        for (; buffer_i < buffer_size_per_tasket; buffer_i += UNROLL_SIZE)
        {
            index = buffer[index];
            index = buffer[index];
            index = buffer[index];
            index = buffer[index];
            index = buffer[index];
            index = buffer[index];
            index = buffer[index];
            index = buffer[index];
        }
        nb_iterations++;

        cycles = perfcounter_get();
        if (cycles > max_cycles)
        {
            break;
        }
    }

    result->cycles = perfcounter_get();
    result->bytes_read = nb_iterations * buffer_size_per_tasket * sizeof(uint32_t);
    return 0;
}
