ENTRY (_start)

SECTIONS {
	. = 0x74B10000;
	.text : {
		*(.text)
	}
	.data : {
		*(.rdata)
		*(.rodata)
		*(.data)
	}
	.bss : {
		PROVIDE(__bss_start = .);
		*(COMMON)
		*(.bss)
		PROVIDE(__bss_end = .);
	}
	PROVIDE(_end = .);
	PROVIDE(__end = .);
}
