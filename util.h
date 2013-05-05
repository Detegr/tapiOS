#include <stdint.h>

typedef struct gdt
{
}__attribute__((packed)) gdt_t;

typedef struct idt
{
}__attribute__((packed)) idt_t;

void setgdt(gdt_t* gdt, uint16_t gdtsize);
void setidt(idt_t* idt, uint16_t idtsize);
