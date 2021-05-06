#include <stdio.h>
#include <perfcounter.h>
#include <mram.h>
#include <defs.h>
#include <barrier.h>

#include "common.h"

#define BLOCK_SIZE 256
#define UNROLL_SIZE 8
#define CACHE 1

__dma_aligned uint32_t caches[NR_TASKLETS][BLOCK_SIZE];
__host dpu_input_t input;
__host dpu_output_t results;
__mram_noinit uint32_t buffer[MAX_BUFFER_SIZE];

int main()
{
    uint32_t tasklet_id = me();
#ifdef CACHE
    uint32_t *cache = caches[tasklet_id];
#endif
    tasklet_output_t *result = &results.tasklet_result[tasklet_id];

    uint32_t total_buffer_size = input.total_buffer_size;
    // This will only be *entirely accurate* if NR_TASKLETS is a power of 2
    uint32_t buffer_size_per_tasket = input.tasklet_buffer_size;
    uint64_t max_cycles = input.max_cycles * CLOCKS_PER_SEC;

    if (tasklet_id == 0)
    {
        perfcounter_config(COUNT_CYCLES, true);
    }

    uint32_t read_val = 0, nb_iterations = 0;
    uint64_t cycles;
    while (1)
    {
        uint32_t buffer_i = input.tasklet_start_index[tasklet_id];
        uint32_t end_i = buffer_i + buffer_size_per_tasket;

#ifdef CACHE
        for (; buffer_i < end_i; buffer_i += BLOCK_SIZE)
        {
            // read MRAM buffer to WRAM cache
            mram_read(&buffer[buffer_i], cache, BLOCK_SIZE * sizeof(uint32_t));

            // unroll loop to have greater ratio of load instructions to branch instructions
            for (uint32_t block_i = 0; block_i < BLOCK_SIZE; block_i += UNROLL_SIZE)
            {
                read_val += cache[block_i + 0];
                read_val += cache[block_i + 1];
                read_val += cache[block_i + 2];
                read_val += cache[block_i + 3];
                read_val += cache[block_i + 4];
                read_val += cache[block_i + 5];
                read_val += cache[block_i + 6];
                read_val += cache[block_i + 7];
            }
        }
#else
        for (; buffer_i < end_i; buffer_i += UNROLL_SIZE)
        {
            read_val += buffer[buffer_i + 0];
            read_val += buffer[buffer_i + 1];
            read_val += buffer[buffer_i + 2];
            read_val += buffer[buffer_i + 3];
            read_val += buffer[buffer_i + 4];
            read_val += buffer[buffer_i + 5];
            read_val += buffer[buffer_i + 6];
            read_val += buffer[buffer_i + 7];
        }
#endif
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
