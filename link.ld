OUTPUT_FORMAT(elf32-i386)
STARTUP(loader.o)
OUTPUT(kernel.bin)

SECTIONS
{
	. = 0x100000; /* Multiboot kernel must not start before 1mb mark */
	.text ALIGN(4096) :
	{
		*(.text)
	}
	.rodata ALIGN(4096) :
	{
		*(.rodata*)
	}
	.data ALIGN(4096) :
	{
		*(.data)
	}
	.bss :
	{
		*(COMMON)
		*(.bss)
	}
}
