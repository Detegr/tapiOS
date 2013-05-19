global _start
extern kmain

; Paging stuff
KERNEL_VMA equ 0xC0000000
PAGE_ENTRIES equ 1024

; From paging.s
extern _map_kernel_to_higher_half

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

STACKSIZE	equ 0x4000			; 16k stack size

[section .setup]
_start:
	mov esp, stack+STACKSIZE	; Setup stack pointer to the bottom of the stack
	sub esp, KERNEL_VMA			; Correct to the physical address of the stack
	mov ebp, esp
	
	push page_tbl_kernel - KERNEL_VMA
	push page_tbl_low - KERNEL_VMA
	push page_directory - KERNEL_VMA
	jmp _map_kernel_to_higher_half

test:
	push ebp
	mov ebp, esp
	mov eax, [ebp+4]
	leave
	ret

[section .text]
kernel:
	cli							; Disable interrupts until we setup gdt and idt
	call kmain					; Start kernel
	cli							; Clear interrupts if kernel exists
.halt:
	hlt							; ..and halt.
	jmp .halt

section .bss align = 4
stack: resb STACKSIZE ; 16k stack on 32-bit alignment

[section .data]
ALIGN 4096

page_directory:
	times PAGE_ENTRIES dd 0
page_tbl_low:
	times PAGE_ENTRIES dd 0
page_tbl_kernel:
	times PAGE_ENTRIES dd 0
