OBJECTS=loader.o util_asm.o kernel.o util.o video.o
CFLAGS=-Wall -Wextra -Werror -nostdlib -nostartfiles -nodefaultlibs -Wno-unused-parameter -m32 -ffreestanding
ASM_FLAGS=-f elf

all: kernel

kernel: $(OBJECTS)
	ld -T link.ld $(OBJECTS) -o tapios/boot/kernel.bin -melf_i386
	grub-mkrescue -o tapios.iso tapios

%.o : %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

%.o : %.s
	nasm $(ASM_FLAGS) $< -o $@

clean:
	-rm *.o tapios/boot/*.bin

.PHONY: all clean
