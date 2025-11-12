CC = gcc
CFLAGS = -Wall 
CFLAGS_DEBUG = -Wall -g3
GENFLAGS = -Wall -O2 -fopenmp -Wformat-truncation=0

OUTDIR = out
TARGET_SEQ = $(OUTDIR)/sequential.out
TARGET_PSV = $(OUTDIR)/parallelSV.out
TARGET_GEN = $(OUTDIR)/generator.out
TARGET_DEBUG = $(OUTDIR)/debug.out
SRC_SEQ = src/sequential.c
SRC_PSV = src/parallelSV.c
SRC_GEN = utils/generator.c
SRC_DEBUG = src/parallelSV.c

.PHONY: all clean dirs

all: dirs $(TARGET_SEQ) $(TARGET_PSV) $(TARGET_GEN) $(TARGET_DEBUG)

dirs:
	mkdir -p $(OUTDIR)

$(TARGET_SEQ): $(SRC_SEQ)
	$(CC) $(CFLAGS) -o $@ $<

$(TARGET_PSV): $(SRC_PSV)
	$(CC) $(CFLAGS) -o $@ $<

$(TARGET_GEN): $(SRC_GEN)
	$(CC) $(GENFLAGS) -o $@ $<

$(TARGET_DEBUG): $(SRC_DEBUG)
	$(CC) $(CFLAGS_DEBUG) -o $@ $<

clean:
	rm -rf $(OUTDIR)
