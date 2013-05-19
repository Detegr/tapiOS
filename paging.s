[section .setup]

global _map_kernel_to_higher_half
global _setup_page_table
global _map_page

KERNEL_VMA equ 0xC0000000
FLAG_PRESENT equ 0x1

extern page_directory
extern page_tbl_low
extern page_tbl_kernel

extern __kernel_start
extern __kernel_end

_map_kernel_to_higher_half:
	pop eax 			; page_directory

	mov ebx, 0x0
	call _setup_page_table

	mov ebx, 0xC0100000
	call _setup_page_table

	pop ebx 			; page_tbl_low
	pop ecx 			; page_tbl_kernel

	; Map low page (0x1000 - 0x100000)
	; eax contains page_directory
	mov ebx, 0x1000		; virtual
	mov ecx, 1023		; count
	mov edx, 0x1000		; physical
	call _map_page

	mov ebx, 0xC0100000	; virtual
	mov ecx, 4096		; count
	mov edx, 0x100000	; physical
	call _map_page

	ret

_setup_page_table:
	push eax
	mov eax, ebx
	call _calculate_directory_position ; to edx
	pop eax
	or ebx, FLAG_PRESENT
	mov [eax + edx * 4], ebx
	ret

_map_page:
	push ebp
	mov esp, ebp

	sub esp, 20
	mov [esp-20], eax ; Directory
	mov [esp-16], ebx ; Virtual address
	mov [esp-12], ecx ; Count
	mov [esp-8] , DWORD 0   ; Mapped bytes
	mov [esp-4] , edx ; Physical address

	mov eax, ebx
	call _calculate_directory_position

	mov eax, [esp-8] ; Original count
	sub eax, ecx	; pages mapped
	push ecx 		; Count
	mov ecx, 4
	mul ecx
	mov ebx, edx 	; mapped bytes
	pop ecx 		; remaining count
	pop edx 		; directory index
	pop eax 		; original count
	add eax, ebx 	; directory + mapped bytes
	pop ebx 		; virtual address

	push ebx
		push ecx
		mov ecx, [eax + edx * 4] 	; get old entry
		and ecx, FLAG_PRESENT
		cmp ecx, 0x0				; if not present
		je __panic 					; jump to panic
		pop ecx
	pop ebx
	
	mov [eax + edx * 4], ebx ; physical address to correct page table
	or DWORD [eax + edx * 4], FLAG_PRESENT
	add ebx, 0x1000 		 ; advance to next page

	pop eax ; directory

	loop _map_page

	add esp, 20
	leave
	ret

__panic:
	hlt
	jmp __panic

_calculate_directory_position:
	mov edx, eax
	shr edx, 22 ; Index in page directory
	ret

_set_kernel_tbl_index:
	or ebx, FLAG_PRESENT
	mov [eax+edx], ebx ; Use index from edx
	ret


_enable_paging:
	ret
