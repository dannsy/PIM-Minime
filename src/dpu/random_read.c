#include <perfcounter.h>
#include <mram.h>
#include <defs.h>
#include <barrier.h>

#include "common.h"

__mram_noinit uint32_t buffer[BUFFER_SIZE];
__host dpu_results_t results;

int main()
{
    uint32_t tasklet_id = me();
    result_t *result = &results.tasklet_result[tasklet_id];

    if (tasklet_id == 0)
    {
        perfcounter_config(COUNT_CYCLES, true);
    }

    uint32_t index = 0, nb_iterations = 0;
    while (1)
    {
        // unroll loop to have greater ratio of load instructions to branch instructions
        for (int i = 0; i < BUFFER_SIZE; i += 8)
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
        break;
    }

    result->cycles = perfcounter_get();
    result->bytes_read = nb_iterations * BUFFER_SIZE * sizeof(uint32_t);
    return 0;
}
