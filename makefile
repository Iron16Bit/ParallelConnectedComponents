CC = gcc
MPICC = mpicc
CFLAGS = -Wall -std=c99
CFLAGS_DEBUG = -Wall -g3 -std=c99
GENFLAGS = -Wall -O2 -fopenmp -std=c99

OUTDIR = out
TARGET_SEQ_REM = $(OUTDIR)/sequentialRem.out
TARGET_SEQ_SV = $(OUTDIR)/sequentialSV.out
TARGET_PSV = $(OUTDIR)/parallelSV.out
TARGET_GEN = utils/generator.out
TARGET_DEBUG = $(OUTDIR)/debug.out

SRC_SEQ_REM = src/sequential/sequentialRem.c
SRC_SEQ_SV = src/sequential/sequentialSV.c
SRC_PSV = src/parallel/parallelSV.c
SRC_GEN = utils/generator.c
SRC_DEBUG = src/parallel/parallelSV.c

.PHONY: all clean dirs

parallel: dirs $(TARGET_PSV)
all: dirs $(TARGET_SEQ_REM) $(TARGET_SEQ_SV) $(TARGET_PSV) $(TARGET_GEN) $(TARGET_DEBUG)

dirs:
	mkdir -p $(OUTDIR)

$(TARGET_SEQ_REM): $(SRC_SEQ_REM)
	$(CC) $(CFLAGS) -o $@ $<

$(TARGET_SEQ_SV): $(SRC_SEQ_SV)
	$(CC) $(CFLAGS) -o $@ $<

$(TARGET_PSV): $(SRC_PSV)
	$(MPICC) $(CFLAGS) -o $@ $<

$(TARGET_GEN): $(SRC_GEN)
	$(CC) $(GENFLAGS) -o $@ $<

debug: dirs $(TARGET_DEBUG)

$(TARGET_DEBUG): $(SRC_DEBUG)
	$(MPICC) $(CFLAGS_DEBUG) -o $@ $<

clean:
	rm -rf $(OUTDIR) $(TARGET_GEN)
