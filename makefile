CC = gcc
MPICC = mpicc
CFLAGS = -Wall -std=c99

OUTDIR = out
TARGET_SEQ_REM = $(OUTDIR)/sequentialRem.out
TARGET_SEQ_SV = $(OUTDIR)/sequentialSV.out
TARGET_PSV = $(OUTDIR)/parallelSV.out

SRC_SEQ_REM = src/sequential/sequentialRem.c
SRC_SEQ_SV = src/sequential/sequentialSV.c
SRC_PSV = src/parallel/parallelSV.c

.PHONY: all clean dirs

parallel: dirs $(TARGET_PSV)
all: dirs $(TARGET_SEQ_REM) $(TARGET_SEQ_SV) $(TARGET_PSV)

dirs:
	mkdir -p $(OUTDIR)

$(TARGET_SEQ_REM): $(SRC_SEQ_REM)
	$(CC) $(CFLAGS) -o $@ $<

$(TARGET_SEQ_SV): $(SRC_SEQ_SV)
	$(CC) $(CFLAGS) -o $@ $<

$(TARGET_PSV): $(SRC_PSV)
	$(MPICC) $(CFLAGS) -o $@ $< -lm -fopenmp

clean:
	rm -rf $(OUTDIR)
