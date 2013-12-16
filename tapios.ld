ENTRY (_start)
INPUT (crt0.o)
GROUP(-lnosys -lc -lnosys)

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
