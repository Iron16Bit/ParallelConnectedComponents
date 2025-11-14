CC = gcc
MPICC = mpicc
CFLAGS = -Wall 
CFLAGS_DEBUG = -Wall -g3
GENFLAGS = -Wall -O2 -fopenmp -Wformat-truncation=0

OUTDIR = out
TARGET_SEQ_REM = $(OUTDIR)/sequentialRem.out
TARGET_SEQ_SV = $(OUTDIR)/sequentialSV.out
TARGET_PSV = $(OUTDIR)/parallelSV.out
TARGET_GEN = utils/generator.out
TARGET_DEBUG = $(OUTDIR)/debug.out

SRC_SEQ_REM = src/sequentialRem.c
SRC_SEQ_SV = src/sequentialSV.c
SRC_PSV = src/parallelSV.c
SRC_GEN = utils/generator.c
SRC_DEBUG = src/parallelSV.c

.PHONY: all clean dirs

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
