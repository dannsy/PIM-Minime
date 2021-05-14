#include <stdio.h>
#include <perfcounter.h>
#include <mram.h>
#include <defs.h>

#include "common.h"

#ifndef BLOCK_SIZE
#define BLOCK_SIZE 2
#endif

__dma_aligned uint8_t caches[NR_TASKLETS][BLOCK_SIZE];
__host dpu_input_t input;
__host dpu_output_t results;
__mram_noinit uint8_t buffer[MAX_BUFFER_SIZE];

int main()
{
    uint32_t tasklet_id = me();
    uint8_t *cache = caches[tasklet_id];
    tasklet_output_t *result = &results.tasklet_result[tasklet_id];

    uint32_t total_buffer_size = input.total_buffer_size;
    uint32_t buffer_size_per_tasklet = input.tasklet_buffer_size;

    uint32_t sum = 0, nb_iterations = 0;

    if (tasklet_id == 0)
    {
        perfcounter_config(COUNT_CYCLES, true);
    }
    uint64_t read_transfer_cycles = 0, write_transfer_cycles = 0;
    uint64_t start_cycle, end_cycle;

    uint32_t buffer_i = input.tasklet_start_index[tasklet_id];
    // uint32_t end_i = buffer_i + BLOCK_SIZE * 1024;

    __mram_ptr void *buffer_addr = (__mram_ptr void *)&buffer;
    start_cycle = perfcounter_get();
    // read MRAM buffer to WRAM buffer
    mram_read(buffer_addr, cache, BLOCK_SIZE);
    end_cycle = perfcounter_get();
    read_transfer_cycles = end_cycle - start_cycle;

    start_cycle = perfcounter_get();
    // write WRAM buffer to MRAM buffer
    mram_write(cache, buffer_addr, BLOCK_SIZE);
    end_cycle = perfcounter_get();
    write_transfer_cycles = end_cycle - start_cycle;

    result->transfer_size = BLOCK_SIZE;
    result->read_cycles = read_transfer_cycles;
    result->write_cycles = write_transfer_cycles;

    result->cycles = perfcounter_get();
    result->bytes_read = buffer_size_per_tasklet * sizeof(uint32_t);
    return 0;
}
