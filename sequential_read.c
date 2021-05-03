#include <stdio.h>
#include <perfcounter.h>

#include <mram.h>
#include <defs.h>
#include <barrier.h>

#include "common.h"

__mram_noinit uint32_t buffer[BUFFER_SIZE];

int main()
{
    printf("Starting sequential read\n");

    perfcounter_config(COUNT_CYCLES, true);
    uint32_t read_val;

    for (int i = 0; i < BUFFER_SIZE; i++)
    {
        read_val = buffer[i];
    }

    perfcounter_t end_time = perfcounter_get();
    printf("Cycles used: %lu\n", end_time);
    return 0;
}
