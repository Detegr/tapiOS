global _setgdt
global _setidt
global _outb
global _io_wait
global _kb_int
global _idle
global _inb
global _noop_int
global _spurious_irq_check_master
global _spurious_irq_check_slave

extern gdtptr
extern idtptr
extern test
extern is_spurious_irq_master
extern is_spurious_irq_slave
extern pic_get_irq
extern panic

section .text

_setgdt:
	lgdt [gdtptr]
	jmp 0x08:flushgdt ; 0x08 is our new code selector in gdt
flushgdt:
	mov ax, 0x10 ; 0x10 is the new data selector
	mov ds,ax
	mov es,ax
	mov fs,ax
	mov gs,ax
	mov ss,ax
	ret

_setidt:
	lidt [idtptr]
	ret

_outb:
	mov al, [esp+8]
	mov dx, [esp+4]
	out dx, al
	ret

_inb:
	mov edx, [esp+4]
	in al, dx
	ret

send_eoi:
	cmp al, 0xFF
	je _panic
	mov bl, al
	mov al, 0x20
	out 0x20, al
	cmp bl, 0x8
	jge send_eoi_slave
send_eoi_fin:
	popad
	iret

send_eoi_slave:
	out 0xA0, al
	jmp send_eoi_fin

_kb_int:
	pushad
	call test
	call pic_get_irq
	jmp send_eoi

_noop_int:
	pushad
	call pic_get_irq
	jmp send_eoi

_spurious_irq_check_master:
	pushad
	call is_spurious_irq_master
	cmp al, 0x0
	je send_eoi
	popad
	iret

_spurious_irq_check_slave:
	pushad
	call is_spurious_irq_slave
	cmp al, 0x0
	je send_eoi
	; Spurious irq to slave, ack master but do not ack slave
	mov al, 0x20
	out 0x20, al
	popad
	iret

_io_wait:
	mov eax, 0x0
	mov dx, 0x3A4 ; 0x3A4 should be unused
	out dx, eax
	ret

_idle:
	hlt
	ret

_panic:
	call panic
	cli
	hlt
