# Makefile for Assignment1_OS

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -pthread -D_POSIX_C_SOURCE=200809L
LDFLAGS = -lrt

TARGETS = producer consumer cleanup
SOURCES = producer.c consumer.c cleanup.c

# Default target
all: $(TARGETS)

# Build each target
producer: producer.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

consumer: consumer.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

cleanup: cleanup.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

# Remove built files
clean:
	rm -f $(TARGETS)
