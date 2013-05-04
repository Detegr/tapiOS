all:
	nasm -f elf32 -o loader.o loader.s
	gcc -o kernel.o -c kernel.c -Wall -Wextra -Werror -nostdlib -nostartfiles -nodefaultlibs -Wno-unused-parameter -m32 -ffreestanding
	ld -T link.ld kernel.o loader.o -o kernel.bin -melf_i386

clean:
	-rm *.o *.bin
