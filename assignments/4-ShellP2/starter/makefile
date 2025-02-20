# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra -g

# Target executable name
TARGET = dsh

# Source files
SRCS = dsh_cli.c dshlib.c
OBJS = $(SRCS:.c=.o)

# Default target
all: clean $(TARGET)

# Link object files to create executable
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

# Compile source files to object files
%.o: %.c dshlib.h
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -f $(TARGET) *.o

test: $(TARGET)
	chmod +x test.sh
	chmod +x bats/*.sh
	./test.sh

# Phony targets
.PHONY: all clean test