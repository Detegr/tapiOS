ASM_SOURCES=boot/loader.s     \
			boot/paging_asm.s \
 			util/util_asm.s   \
			irq/irq_asm.s
SOURCES= terminal/vga.c      \
		irq/gdt.c           \
		irq/idt.c           \
		irq/pic.c           \
		irq/irq.c           \
		util/util.c         \
		mem/kmalloc.c       \
		mem/pmm.c           \
		mem/vmm.c           \
		kmain.c             \
		util/scancodes.c    \
		task/process.c      \
		task/multitasking.c \
		irq/timer.c         \
		syscall/syscalls.c  \
		fs/vfs.c            \
		fs/ext2.c           \
		task/processtree.c  \
		task/scheduler.c    \
		fs/devfs.c          \
		drivers/tty.c       \
		drivers/keyboard.c  \
		dev/pci.c           \
		drivers/rtl8139.c   \
		network/ethernet.c  \
		network/netdev.c    \
		network/ipv4.c      \
		network/tcp.c       \
		network/arp.c       \
		network/socket.c    \
		util/list.c
ASM_OBJECTS=$(ASM_SOURCES:.s=.o)
OBJECTS=$(SOURCES:.c=.o)
DEPS=$(SOURCES:.c=.d)

CFLAGS=-MD -MP -Wall -Wextra -Werror -nostdlib -Wno-unused-parameter -m32 -ffreestanding -g3 -std=c11 -Wno-unused-variable -masm=intel -Wno-unused-function -Wno-unused-but-set-parameter -I. -Wno-address -Wno-sign-compare -Icross/i586-pc-tapios/include
ASM_FLAGS=-f elf -g
LFLAGS=-melf_i386 -Lcross/i586-pc-tapios/lib

.PHONY: all clean iso

all: kernel
iso:
	grub-mkrescue -d/usr/lib/grub/i386-pc -o tapios.iso tapios
kernel: $(ASM_OBJECTS) $(OBJECTS)
	ld -T boot/link.ld $^ -o tapios/boot/kernel.bin $(LFLAGS)
	grub-mkrescue -d/usr/lib/grub/i386-pc -o tapios.iso tapios
userspace:
	make -C usr
	make iso
%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@
%.o : %.s
	nasm $(ASM_FLAGS) $< -o $@
clean:
	-rm $(ASM_OBJECTS)
	-rm $(OBJECTS)
	-rm $(DEPS)
	-rm tapios/boot/*.bin

-include $(DEPS)
