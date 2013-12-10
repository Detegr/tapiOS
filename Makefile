OBJECTS=loader.o paging_asm.o util_asm.o vga.o irq_asm.o irq.o util.o pmm.o vmm.o kernel.o heap.o scancodes.o process.o timer.o syscalls.o
CFLAGS=-Wall -Wextra -Werror -nostdlib -nostartfiles -nodefaultlibs -Wno-unused-parameter -m32 -ffreestanding -g3 -std=c99 -Wno-unused-variable -masm=intel -fno-builtin -fno-stack-protector -Wno-unused-function
ASM_FLAGS=-f elf -g
LFLAGS=-melf_i386

all: kernel

kernel: $(OBJECTS)
	ld -T link.ld $(OBJECTS) -o tapios/boot/kernel.bin $(LFLAGS)
	grub-mkrescue -o tapios.iso tapios

%.o : %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@ -g3

%.o : %.s
	nasm $(ASM_FLAGS) $< -o $@

clean:
	-rm *.o tapios/boot/*.bin

.PHONY: all clean
