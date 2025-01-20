# Compiler, assembler, and linker
CC = gcc
LD = ld
ASM = nasm

# Flags for compilation, assembly, and linking
CFLAGS = -m32 -c
ASMFLAGS = -f elf32
LDFLAGS = -L/usr/lib32 -lc -T linking_script -dynamic-linker /lib32/ld-linux.so.2

# Source and object files
SOURCES = my_loader.c
ASM_FILES = startup.s start.s
OBJECTS = $(SOURCES:.c=.o) $(ASM_FILES:.s=.o)

# Output binary
TARGET = my_loader

# Default target
all: $(TARGET)

# Rule to build the target
$(TARGET): $(OBJECTS)
	$(LD) -o $@ $(OBJECTS) $(LDFLAGS)

# Rule to compile C source files to object files
%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

# Rule to assemble assembly files to object files using nasm
%.o: %.s
	$(ASM) $(ASMFLAGS) $< -o $@

# Clean target to remove generated files
clean:
	rm -f $(TARGET) $(OBJECTS)

# Phony targets
.PHONY: all clean
