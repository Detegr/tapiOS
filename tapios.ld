ENTRY (_start)
INPUT (crt0.o)

SECTIONS {
	. = 0x74B10000;
	.text : {
		*(.text)
		*(.rdata)
		*(.rodata)
		*(.data)
	}
	.bss : {
		_sbss = .;
		*(COMMON)
		*(.bss)
		_ebss = .;
	}
}
