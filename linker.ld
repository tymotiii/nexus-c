ENTRY(_start)

SECTIONS
{
    /* GRUB ładuje jądro od adresu 1 MB */
    . = 1M;

    .text :
    {
        *(.multiboot)   /* Nagłówek ląduje PIERWSZY */
        *(.text)        /* Potem właściwy kod funkcji */
        *(.rodata*)     /* Tutaj trafiają teksty w cudzysłowach, np. "Siemanko..." */
    }

    .data : { *(.data) }
    .bss  : { *(.bss) }
}
