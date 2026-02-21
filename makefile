CFLAGS = -Wall -Wextra -Wpedantic -std=c99 -Iinclude
LDFLAGS =
SRCDIR = src
SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(SOURCES:.c=.o)
TARGET_BASE = ascii-view

ifeq ($(OS),Windows_NT)
CC = gcc
TARGET = $(TARGET_BASE).exe
else
CC ?= gcc
TARGET = $(TARGET_BASE)
CFLAGS += -D_GNU_SOURCE
LDFLAGS += -lm
endif

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $(TARGET)

# Release build with optimization
release: CFLAGS += -O3 -flto -march=native
release: LDFLAGS += -flto
release: clean $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(SRCDIR)/*.o ascii-view ascii-view.exe

.PHONY: clean release
