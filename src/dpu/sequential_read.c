#include <stdio.h>
#include <perfcounter.h>
#include <mram.h>
#include <defs.h>
#include <alloc.h>

#include "common.h"

#ifndef BLOCK_SIZE
#define BLOCK_SIZE 1024
#endif
#define STRIDE 1

__dma_aligned uint8_t CACHE0[NR_TASKLETS][BLOCK_SIZE];
__dma_aligned uint8_t CACHE1[NR_TASKLETS][BLOCK_SIZE];

__host dpu_input_t input;
__host dpu_output_t results;
__mram_noinit uint8_t buffer[MAX_BUFFER_SIZE];

int main()
{
    uint32_t tasklet_id = me();

    // read from MRAM and store in cache0, and then read from cache0 and store in cache1
    uint8_t *cache0 = CACHE0[tasklet_id];
    uint8_t *cache1 = CACHE1[tasklet_id];

    tasklet_output_t *result = &results.tasklet_result[tasklet_id];

    uint32_t total_buffer_size = input.total_buffer_size;
    // This will only be *entirely accurate* if NR_TASKLETS is a power of 2
    uint32_t buffer_size_per_tasket = input.tasklet_buffer_size;
    uint64_t max_cycles = input.bench_time * CLOCKS_PER_SEC;

    if (tasklet_id == 0)
    {
        perfcounter_config(COUNT_CYCLES, true);
    }

    uint32_t nb_iterations = 0;
    uint64_t cycles;
    while (1)
    {
        uint32_t buffer_i = input.tasklet_start_index[tasklet_id];
        uint32_t end_i = buffer_i + buffer_size_per_tasket;

        for (; buffer_i < end_i; buffer_i += BLOCK_SIZE)
        {
            mram_read(&buffer[buffer_i], cache0, BLOCK_SIZE);

            // for (int i = 0; i < BLOCK_SIZE; i += STRIDE) {
            //     cache1[i] = cache0[i];
            // }
            // cache1[0] = cache0[0] + 10;

            // mram_write(cache, &buffer[buffer_i], BLOCK_SIZE);
        }
        nb_iterations++;

        cycles = perfcounter_get();
        if (cycles > max_cycles)
        {
            break;
        }
    }
    printf("%u %u\n", total_buffer_size, buffer_size_per_tasket);

    result->cycles = perfcounter_get();
    result->bytes_read = nb_iterations * buffer_size_per_tasket / STRIDE;

    // int *bufferA = mem_alloc(256 * sizeof(int));
    // uint32_t start_cycles = perfcounter_get();
    // for (int i = 0; i < 256; i++) {
    //     int temp = bufferA[i];
    //     temp += 4;
    //     bufferA[i] = temp;
    // }
    // uint32_t end_cycles = perfcounter_get();
    // printf("Cycles: %u\n", end_cycles - start_cycles);

    return 0;
}
