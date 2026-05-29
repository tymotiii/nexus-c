; Multiboot 1 header must be in the first 8 KiB of the kernel file.
%define MULTIBOOT_MAGIC    0x1BADB002
%define MULTIBOOT_FLAGS    0x00000003
%define MULTIBOOT_CHECKSUM -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

section .multiboot
align 4
    dd MULTIBOOT_MAGIC
    dd MULTIBOOT_FLAGS
    dd MULTIBOOT_CHECKSUM

section .text
global _start
extern kmain

_start:
    push ebx                ; multiboot info (2nd arg)
    push eax                ; multiboot magic (1st arg)
    call kmain
.hang:
    hlt
    jmp .hang
