#include <stdio.h>
#include <perfcounter.h>

#include <mram.h>
#include <defs.h>
#include <barrier.h>

#include "common.h"

__mram_noinit uint32_t buffer[BUFFER_SIZE];

int main()
{
    printf("Starting random read\n");

    perfcounter_config(COUNT_CYCLES, true);
    uint32_t read_val;

    int index = 0;
    for (int i = 0; i < BUFFER_SIZE; i++)
    {
        index = buffer[index];
    }

    perfcounter_t end_time = perfcounter_get();
    printf("Cycles used: %lu\n", end_time);
    return 0;
}
