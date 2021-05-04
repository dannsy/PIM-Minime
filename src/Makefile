IDIR=../include
CC=gcc
CFLAGS=-I$(IDIR) -g -Wall -Werror
DPU_OPTS = `dpu-pkg-config --cflags --libs dpu`

ODIR=obj

_DEPS=common.h
DEPS=$(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ=host.o
OBJ=$(patsubst %,$(ODIR)/%,$(_OBJ))

# Default
NR_DPUS ?= 1
NR_TASKLETS ?= 1
CFLAGS+=-DNR_DPUS=$(NR_DPUS) -DNR_TASKLETS=$(NR_TASKLETS) -DHOST=1

$(ODIR)/%.o: %.c  $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) $(DPU_OPTS)

host: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(DPU_OPTS)
	$(MAKE) -C dpu

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o
