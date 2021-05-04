IDIR=../../include
CC=dpu-upmem-dpurte-clang
CFLAGS+=-DNR_DPUS=$(NR_DPUS) -DNR_TASKLETS=$(NR_TASKLETS)

default: all

all: sequential_read random_read

clean:
	$(RM) $(HDC_DPU)

sequential_read: sequential_read.c
	$(CC) -o $@ $^ $(CLAGS) -I$(IDIR)

random_read: random_read.c
	$(CC) -o $@ $^ $(CLAGS) -I$(IDIR)
