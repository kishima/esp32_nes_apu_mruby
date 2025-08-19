# NSF Player Makefile

CC = gcc
CFLAGS = -Wall -Wextra -O2 -g
TARGET = nsf_player

# Source files
SRCS = main.c nsf.c cpu6502.c apu_stub.c
OBJS = $(SRCS:.c=.o)

# Default target
all: $(TARGET)

# Build the executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^
	@echo "Build complete: $(TARGET)"

# Compile source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -f $(OBJS) $(TARGET)
	@echo "Clean complete"

# Run with test file
test: $(TARGET)
	./$(TARGET) ../temp/dq.nsf

# Run with debug
debug: $(TARGET)
	gdb ./$(TARGET)

# Help
help:
	@echo "Available targets:"
	@echo "  all    - Build the NSF player"
	@echo "  clean  - Remove build files"
	@echo "  test   - Run with test NSF file"
	@echo "  debug  - Run with gdb"
	@echo "  help   - Show this help"

.PHONY: all clean test debug help