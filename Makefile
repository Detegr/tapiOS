all:
	nasm -f elf -o loader.o loader.s
	nasm -f elf -o util_asm.o util.s
	gcc -o kernel.o -c kernel.c -Wall -Wextra -Werror -nostdlib -nostartfiles -nodefaultlibs -Wno-unused-parameter -m32 -ffreestanding
	gcc -o util.o -c util.c -Wall -Wextra -Werror -nostdlib -nostartfiles -nodefaultlibs -Wno-unused-parameter -m32 -ffreestanding
	ld -T link.ld kernel.o util.o util_asm.o loader.o -o tapios/boot/kernel.bin -melf_i386
	grub-mkrescue -o tapios.iso tapios

clean:
	-rm *.o tapios/boot/*.bin
