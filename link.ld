OUTPUT_FORMAT(elf32-i386)
OUTPUT(kernel.bin)
ENTRY(_start)

SECTIONS
{
	VIRTUAL_BASE = 0xC0000000;

	. = 0x100000; /* Multiboot kernel must not start before 1mb mark */
	.setup : /* Link setup to 1mb so we can get a linear address for initial GDT */
	{
		*(.setup)
	}

	__kernel_start = .;
	. += VIRTUAL_BASE;

	.text : AT(ADDR(.text) - VIRTUAL_BASE)
	{
		*(.text)
	}
	.data ALIGN(4096) : AT(ADDR(.data) - VIRTUAL_BASE)
	{
		*(.data)
		*(.rodata*)
	}
	.eh_frame ALIGN(4096) : AT(ADDR(.eh_frame) - VIRTUAL_BASE)
	{
		*(.eh_frame);
		*(.rel_eh_frame);
	}
	.bss ALIGN(4096) : AT(ADDR(.bss) - VIRTUAL_BASE)
	{
		*(COMMON*)
		*(.bss*)
	}
	__kernel_end = .;
}
