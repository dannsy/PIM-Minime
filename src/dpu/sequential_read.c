#include <perfcounter.h>
#include <mram.h>
#include <defs.h>
#include <barrier.h>

#include "common.h"

#define BLOCK_SIZE 256

__dma_aligned uint32_t caches[NR_TASKLETS][BLOCK_SIZE];
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
        for (uint32_t buffer_i = tasklet_id * BLOCK_SIZE; buffer_i < BUFFER_SIZE; buffer_i += (NR_TASKLETS * BLOCK_SIZE))
        {
            // read MRAM buffer to WRAM cache
            mram_read(&buffer[buffer_i], cache, BLOCK_SIZE * sizeof(uint32_t));

            // unroll loop to have greater ratio of load instructions to branch instructions
            for (uint32_t block_i = 0; block_i < BLOCK_SIZE; block_i += 8)
            {
                read_val = cache[block_i + 0];
                read_val = cache[block_i + 1];
                read_val = cache[block_i + 2];
                read_val = cache[block_i + 3];
                read_val = cache[block_i + 4];
                read_val = cache[block_i + 5];
                read_val = cache[block_i + 6];
                read_val = cache[block_i + 7];
            }
        }
        nb_iterations++;
        break;
    }

    result->cycles = perfcounter_get();
    result->bytes_read = nb_iterations * BUFFER_SIZE * sizeof(uint32_t);
    return 0;
}
