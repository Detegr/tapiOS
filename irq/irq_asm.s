%macro IRQ_HANDLER 2
	global %1
	extern %2
	%1:
	cli
	pushad
	push ds
	push es
	push fs
	push gs
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	call %2
	call pic_get_irq
	pop gs
	pop fs
	pop es
	pop ds
	jmp send_eoi
%endmacro

global _inb
global _noop_int
global _spurious_irq_check_master
global _spurious_irq_check_slave
global send_eoi

extern _panic
extern is_spurious_irq_master
extern is_spurious_irq_slave
extern pic_get_irq

IRQ_HANDLER _timer_handler, timer_handler
IRQ_HANDLER _page_fault, page_fault

global _generic_isr
extern generic_isr
_generic_isr:
	cli
	pushad
	push ds
	push es
	push fs
	push gs
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	call generic_isr
	call pic_get_irq
	pop gs
	pop fs
	pop es
	pop ds
	jmp send_eoi

global _syscall
extern syscall
_syscall:
	cli
	pushad
	push ds
	push es
	push fs
	push gs
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	call syscall
	mov [esp+44], eax ; Replace old eax with return value
	pop gs
	pop fs
	pop es
	pop ds
	popad
	iret

send_eoi:
	cmp al, 0xFF
	je send_eoi_fin ; EOI already handled in the handler
	mov bl, al
	mov al, 0x20
	out 0x20, al
	cmp bl, 0x8
	jge send_eoi_slave
send_eoi_fin:
	popa
	iret

send_eoi_slave:
	out 0xA0, al
	jmp send_eoi_fin

_noop_int:
	pushad
	call pic_get_irq
	jmp send_eoi

_spurious_irq_check_master:
	pushad
	call is_spurious_irq_master
	cmp al, 0x0
	je send_eoi ; IRQ8 handler goes here
	popad
	iret

_spurious_irq_check_slave:
	pushad
	call is_spurious_irq_slave
	cmp al, 0x0
	je send_eoi ; IRQ 15 handler goes here
	; Spurious irq to slave, ack master but do not ack slave
	mov al, 0x20
	out 0x20, al
	popad
	iret
