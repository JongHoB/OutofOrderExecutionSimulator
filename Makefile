# Compiler and compiler flags
CC = gcc
CFLAGS = -g -O2

# Output executable name
TARGET = cse561sim

# Default target to build the executable
all: $(TARGET)

# Link the object file to create the executable
$(TARGET): cse561sim.c cse561sim.h
	$(CC) $(CFLAGS) cse561sim.c -o $(TARGET)

# Clean up generated executable
clean:
	rm -f $(TARGET)
