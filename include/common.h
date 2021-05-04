#ifndef COMMON_H_
#define COMMON_H_

/*
 * The maximum buffer size for each DPU is must be smaller than 64MB.
 * Each buffer element is 32 bits = 4 bytes, so if the buffer size is 32MB,
 * then the buffer needs 8 * 1024 * 1024 elements
 */
#define BUFFER_SIZE 8 * 1024 * 1024

typedef struct
{
    const char *name;
} memory_bench_plugin_t;

#endif // COMMON_H_