OBJECTS=loader.o paging_asm.o util_asm.o vga.o irq_asm.o irq.o util.o pmm.o vmm.o kernel.o heap.o scancodes.o process.o timer.o syscalls.o fs/vfs.o fs/ext2.o
CFLAGS=-Wall -Wextra -Werror -nostdlib -Wno-unused-parameter -m32 -ffreestanding -g3 -std=c99 -Wno-unused-variable -masm=intel -Wno-unused-function -Wno-unused-but-set-parameter
ASM_FLAGS=-f elf -g
LFLAGS=-melf_i386

all: kernel

iso:
	grub-mkrescue -o tapios.iso tapios

kernel: $(OBJECTS)
	nasm -f elf crt0.s
	ld -T link.ld $(OBJECTS) -o tapios/boot/kernel.bin $(LFLAGS)
	grub-mkrescue -o tapios.iso tapios

%.o : %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@ -g3

%.o : %.s
	nasm $(ASM_FLAGS) $< -o $@

clean:
	-rm *.o tapios/boot/*.bin

.PHONY: all clean
