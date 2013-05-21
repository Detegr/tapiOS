OBJECTS=paging.o loader.o paging_asm.o util_asm.o irq_asm.o irq.o util.o video.o kernel.o
CFLAGS=-Wall -Wextra -Werror -nostdlib -nostartfiles -nodefaultlibs -Wno-unused-parameter -m32 -ffreestanding -g3
ASM_FLAGS=-f elf
LFLAGS=-melf_i386

all: kernel

kernel: $(OBJECTS)
	ld -T link.ld $(OBJECTS) -o tapios/boot/kernel.bin $(LFLAGS)
	grub-mkrescue -o tapios.iso tapios

%.o : %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

%.o : %.s
	nasm $(ASM_FLAGS) $< -o $@

clean:
	-rm *.o tapios/boot/*.bin

.PHONY: all clean
