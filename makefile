CC = gcc
CFLAGS = -Wall

OUTDIR = out
TARGET = $(OUTDIR)/sequential.out
SRC = src/sequential.c

.PHONY: all clean dirs

all: dirs $(TARGET)

dirs:
	mkdir -p $(OUTDIR)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGET)