OUTPUT_FORMAT(elf32-i386)
STARTUP(loader.o)
OUTPUT(kernel.bin)

SECTIONS
{
	. = 0x100000; /* Multiboot kernel must not start before 1mb mark */
	.text : ALIGN(0x1000) { /* Align sections at page boundaries */
		*(.text)
	}
	.data : ALIGN(0x1000) {
		*(.data)
	}

	.bss : ALIGN(0x1000) {
		*(.bss)
	}
}
