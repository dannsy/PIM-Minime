#include <perfcounter.h>
#include <mram.h>
#include <defs.h>
#include <barrier.h>

#include "common.h"

__mram_noinit uint32_t buffer[MAX_BUFFER_SIZE];
__host dpu_input_t input;
__host dpu_output_t results;

int main()
{
    uint32_t tasklet_id = me();
    tasklet_output_t *result = &results.tasklet_result[tasklet_id];
    uint32_t buffer_size = input.buffer_size;

    if (tasklet_id == 0)
    {
        perfcounter_config(COUNT_CYCLES, true);
    }

    uint32_t index = 0, nb_iterations = 0;
    while (1)
    {
        // unroll loop to have greater ratio of load instructions to branch instructions
        for (int i = 0; i < buffer_size; i += 8)
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
    result->bytes_read = nb_iterations * buffer_size * sizeof(uint32_t);
    return 0;
}
