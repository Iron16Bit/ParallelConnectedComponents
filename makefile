CC = gcc
CFLAGS = -Wall
GENFLAGS = -Wall -O2 -fopenmp -Wformat-truncation=0

OUTDIR = out
TARGET_SEQ = $(OUTDIR)/sequential.out
TARGET_GEN = $(OUTDIR)/generator.out
SRC_SEQ = src/sequential.c
SRC_GEN = utils/generator.c

.PHONY: all clean dirs

all: dirs $(TARGET_SEQ) $(TARGET_GEN)

dirs:
	mkdir -p $(OUTDIR)

$(TARGET_SEQ): $(SRC_SEQ)
	$(CC) $(CFLAGS) -o $@ $<

$(TARGET_GEN): $(SRC_GEN)
	$(CC) $(GENFLAGS) -o $@ $<

clean:
	rm -f $(TARGET_SEQ) $(TARGET_GEN)
