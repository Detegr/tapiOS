OBJECTS=boot/loader.o boot/paging_asm.o util/util_asm.o terminal/vga.o irq/gdt.o irq/idt.o irq/pic.o irq/irq_asm.o irq/irq.o util/util.o mem/kmalloc.o mem/pmm.o mem/vmm.o kmain.o util/scancodes.o task/process.o task/multitasking.o irq/timer.o syscall/syscalls.o fs/vfs.o fs/ext2.o task/processtree.o task/scheduler.o fs/devfs.o drivers/tty.o drivers/keyboard.o dev/pci.o drivers/rtl8139.o network/ethernet.o network/netdev.o network/ipv4.o network/tcp.o
CFLAGS=-Wall -Wextra -Werror -nostdlib -Wno-unused-parameter -m32 -ffreestanding -g3 -std=c99 -Wno-unused-variable -masm=intel -Wno-unused-function -Wno-unused-but-set-parameter -I. -Wno-address -Wno-sign-compare -Icross/i586-pc-tapios/include
ASM_FLAGS=-f elf -g
LFLAGS=-melf_i386 -Lcross/i586-pc-tapios/lib

all: kernel

iso:
	grub-mkrescue -d/usr/lib/grub/i386-pc -o tapios.iso tapios

kernel: $(OBJECTS)
	ld -T boot/link.ld $(OBJECTS) -o tapios/boot/kernel.bin $(LFLAGS)
	grub-mkrescue -d/usr/lib/grub/i386-pc -o tapios.iso tapios

userspace:
	make -C usr
	make iso

%.o : %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@ -g3

%.o : %.s
	nasm $(ASM_FLAGS) $< -o $@

clean:
	-rm irq/*.o
	-rm mem/*.o
	-rm task/*.o
	-rm boot/*.o
	-rm fs/*.o
	-rm terminal/*.o
	-rm syscall/*.o
	-rm util/*.o
	-rm network/*.o
	-rm *.o tapios/boot/*.bin

.PHONY: all clean
