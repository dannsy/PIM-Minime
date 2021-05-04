#include <stdio.h>
#include <perfcounter.h>
#include <mram.h>
#include <defs.h>
#include <barrier.h>

#include "common.h"

#define BLOCK_SIZE 256

__dma_aligned uint32_t caches[NR_TASKLET][BLOCK_SIZE];
__host dpu_results_t results;
__mram_noinit uint32_t buffer[BUFFER_SIZE];

int main()
{
    uint32_t tasklet_id = me();
    uint32_t *cache = caches[tasklet_id];
    result_t *result = &results.tasklet_result[tasklet_id];

    if (tasklet_id == 0)
    {
        perfcounter_config(COUNT_CYCLES, true);
    }

    uint32_t read_val = 0, nb_iterations = 0;
    while (1)
    {
        for (uint32_t buffer_i = tasklet_id * BLOCK_SIZE; buffer_i < BUFFER_SIZE; buffer_i += (NR_TASKLET * BLOCK_SIZE))
        {
            // read MRAM buffer to WRAM cache
            mram_read(&buffer[buffer_i], cache, BLOCK_SIZE);

            for (uint32_t block_i = 0; block_i < BLOCK_SIZE; block_i++)
            {
                read_val += cache[block_i];
            }
        }
        nb_iterations++;
        break;
    }

    result->cycles = perfcounter_get();
    result->bytes_read = nb_iterations * BUFFER_SIZE * sizeof(uint32_t);
    printf("Sum: %u, Cycles: %lu", read_val, result->cycles);
    return 0;
}
