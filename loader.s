extern kmain

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

gdt dw 0
	dd 0

idt dw 0
	dd 0

STACKSIZE	equ 0x4000			; 16k stack size

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
