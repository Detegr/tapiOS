global _start
global _page_directory
extern kmain
                                           ; Paging stuff
KERNEL_VMA equ 0xC0000000
PAGE_ENTRIES equ 1024

                                           ; Multiboot header
MODULEALIGN equ 0x1                        ; Align modules on page boundaries
MEMINFO		equ 0x2                        ; Information on available memory must be included
FLAGS		equ MODULEALIGN|MEMINFO
MAGIC		equ 0x1BADB002                 ; Multiboot header magic
CHECKSUM	equ 0-(MAGIC + FLAGS)

                                           ; From paging.s
extern _setup_page_table
extern _map_page
extern _enable_paging

section .text align = 4

                                           ; Allocate multiboot header
dd MAGIC
dd FLAGS
dd CHECKSUM

STACKSIZE	equ 0x4000                     ; 16k stack size

[section .setup]
_start:
	mov esp, stack+STACKSIZE               ; Setup stack pointer to the bottom of the stack
	sub esp, KERNEL_VMA                    ; Correct to the physical address of the stack
	mov ebp, esp

	push _page_tbl_kernel - KERNEL_VMA
	push _page_tbl_low - KERNEL_VMA
	push _page_directory - KERNEL_VMA

	jmp _map_kernel_to_higher_half

_map_kernel_to_higher_half:
	pop eax                                ; page_directory

	mov ebx, 0x0
	pop ecx                                ; page_tbl_low
	call _setup_page_table

	mov ebx, 0xC0000000
	pop ecx                                ; page_tbl_kernel
	call _setup_page_table

										   ; Identity map first 4mb
	                                       ; eax contains page_directory
	mov ebx, 0x0                           ; virtual
	mov ecx, 1024                          ; count
	mov edx, 0x0                           ; physical
	call _map_page

										   ; Map kernel 0mb - 4mb to 0xC0000000
	mov ebx, 0xC0000000                    ; virtual
	mov ecx, 1024                          ; count
	mov edx, 0x0                           ; physical
	call _map_page

	call _enable_paging
	jmp kernel

[section .text]
kernel:
	xor ebp, ebp
	cli                                    ; Disable interrupts until we setup gdt and idt
	call kmain                             ; Start kernel
	cli                                    ; Clear interrupts if kernel exists
.halt:
	hlt                                    ; ..and halt.
	jmp .halt

section .bss align = 4
stack: resb STACKSIZE                      ; 16k stack on 32-bit alignment

[section .data]
ALIGN 4096

_page_directory:
	times PAGE_ENTRIES dd 0
_page_tbl_low:
	times PAGE_ENTRIES dd 0
_page_tbl_kernel:
	times PAGE_ENTRIES dd 0
