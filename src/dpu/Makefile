IDIR=../../include
CC=dpu-upmem-dpurte-clang
CFLAGS=-DNR_DPUS=$(NR_DPUS) -DNR_TASKLETS=$(NR_TASKLETS) -I$(IDIR)

.PHONY: clean

all: sequential_read random_read

sequential_read: sequential_read.c
	$(CC) $(CFLAGS) $^ -o $@

random_read: random_read.c
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f sequential_read random_read
