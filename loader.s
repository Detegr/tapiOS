extern kmain

; Multiboot header
MODULEALIGN equ 0x1				; Align modules on page boundaries
MEMINFO		equ 0x2				; Information on available memory must be included
FLAGS		equ MODULEALIGN|MEMINFO
MAGIC		equ 0x1BADB002		; Multiboot header magic
CHECKSUM	equ -(MAGIC + FLAGS)
STACKSIZE	equ 0x4000			; 16k stack size

section .text
align 4							; Header must be 32-bit aligned

; Allocate multiboot header
dd MAGIC
dd FLAGS
dd CHECKSUM

_start:
	mov esp, stack+STACKSIZE	; Setup stack pointer to the bottom of the stack
	call kmain					; Start kernel
	cli							; Clear interrupts (when kernel exits)
.halt:
	hlt							; And halt forever
	jmp .halt

section .bss
align 4 ; Align 32-bit
stack: resb STACKSIZE ; 16k stack on 32-bit alignment
