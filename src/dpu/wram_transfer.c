#include <stdio.h>
#include <perfcounter.h>
#include <mram.h>
#include <defs.h>

#include "common.h"

#ifndef BLOCK_SIZE
#define BLOCK_SIZE 2
#endif

__dma_aligned uint32_t caches[NR_TASKLETS][BLOCK_SIZE];
__host dpu_input_t input;
__host dpu_output_t results;
__mram_noinit uint32_t buffer[MAX_BUFFER_SIZE];

int main()
{
    uint32_t tasklet_id = me();
    uint32_t *cache = caches[tasklet_id];
    tasklet_output_t *result = &results.tasklet_result[tasklet_id];

    uint32_t total_buffer_size = input.total_buffer_size;
    uint32_t buffer_size_per_tasklet = input.tasklet_buffer_size;

    uint32_t sum = 0, nb_iterations = 0;

    if (tasklet_id == 0)
    {
        perfcounter_config(COUNT_CYCLES, true);
    }
    uint64_t transfer_cycles = 0;
    uint64_t start_cycle, end_cycle;

    uint32_t buffer_i = input.tasklet_start_index[tasklet_id];
    // uint32_t end_i = buffer_i + BLOCK_SIZE * 1024;

    __mram_ptr void *buffer_addr = (__mram_ptr void *)&buffer;
    uint32_t transfer_size = BLOCK_SIZE * sizeof(uint32_t);
    start_cycle = perfcounter_get();
    // read MRAM buffer to WRAM cache
    mram_read(buffer_addr, cache, transfer_size);
    end_cycle = perfcounter_get();
    transfer_cycles = end_cycle - start_cycle;

    printf("Transfer cycles: %lu, transfer size: %lu bytes\n", transfer_cycles, transfer_size);
    printf("Sum value: %u\n", sum);

    result->cycles = perfcounter_get();
    result->bytes_read = buffer_size_per_tasklet * sizeof(uint32_t);
    return 0;
}
