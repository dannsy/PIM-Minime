IDIR=../../include
CC=dpu-upmem-dpurte-clang
CFLAGS=-DNR_DPUS=$(NR_DPUS) -DNR_TASKLETS=$(NR_TASKLETS) -DBLOCK_SIZE=$(BLOCK_SIZE) -I$(IDIR) -O2

.PHONY: clean

all: sequential_read random_read wram_transfer

sequential_read: sequential_read.c
	$(CC) $(CFLAGS) $^ -o $@

random_read: random_read.c
	$(CC) $(CFLAGS) $^ -o $@

wram_transfer: wram_transfer.c
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f sequential_read random_read wram_transfer
