CC=gcc
CFLAGS=-g -std=gnu11 -Wall -Werror -fopenmp
LDFLAGS=-fopenmp

DIR=src
S=$(shell find $(DIR) -name '*.c' -print)
O=$(patsubst %.c,%.o,$(S))

TARGET=main
all: $(TARGET)

main: $(O)
	$(CC) -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(TARGET) $(O) Makefile.deps

-include Makefile.deps

Makefile.deps:
	$(CC) $(CFLAGS) -MM src/*.[ch] > Makefile.deps
