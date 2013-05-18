extern kmain

; Paging stuff
KERNEL_VMA equ 0xC0000000
PAGE_ENTRIES equ 1024

; From paging.s
extern _enable_paging

; Multiboot header
MODULEALIGN equ 0x1				; Align modules on page boundaries
MEMINFO		equ 0x2				; Information on available memory must be included
FLAGS		equ MODULEALIGN|MEMINFO
MAGIC		equ 0x1BADB002			; Multiboot header magic
CHECKSUM	equ 0-(MAGIC + FLAGS)

section .text align = 4

; Allocate multiboot header
dd MAGIC
dd FLAGS
dd CHECKSUM

page_directory:
	times PAGE_ENTRIES dd 0
page_tbl_low:
	times PAGE_ENTRIES dd 0
page_tbl_kernel:
	times PAGE_ENTRIES dd 0

STACKSIZE	equ 0x4000			; 16k stack size

_start:

kernel:
	mov esp, stack+STACKSIZE	; Setup stack pointer to the bottom of the stack
	cli							; Disable interrupts until we setup gdt and idt
	call kmain					; Start kernel
	cli							; Clear interrupts if kernel exists
.halt:
	hlt							; ..and halt.
	jmp .halt

section .bss align = 4
stack: resb STACKSIZE ; 16k stack on 32-bit alignment
