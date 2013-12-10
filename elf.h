#ifndef _TAPIOS_ELF_H_
#define _TAPIOS_ELF_H_
typedef struct elf_header
{
	uint32_t ident_magic;
	uint32_t ident_info;
	uint64_t unused;
	uint32_t machine_info;
	uint32_t version;
	uint32_t entry;
	uint32_t program_table;
	uint32_t section_table;
	uint32_t flags;
	uint16_t elf_header_size;
	uint16_t program_table_entry_size;
	uint16_t program_entries;
	uint16_t section_table_entry_size;
	uint16_t section_entries;
	uint16_t section_names_index;
} __attribute__((packed)) elf_header;

typedef struct {
	uint32_t p_type;
	uint32_t p_offset;
	uint32_t p_vaddr;
	uint32_t p_paddr;
	uint32_t p_filesz;
	uint32_t p_memsz;
	uint32_t p_flags;
	uint32_t p_align;
} elf_program_entry;

typedef struct
{
	uint32_t sh_name;
	uint32_t sh_type;
	uint32_t sh_flags;
	uint32_t sh_addr;
	uint32_t sh_offset;
	uint32_t sh_size;
	uint32_t sh_link;
	uint32_t sh_info;
	uint32_t sh_addralign;
	uint32_t sh_entsize;
} elf_section_entry;
#endif
