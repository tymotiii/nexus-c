bits 32

global isr_common_stub
global pop_and_iret       ; <--- NOWOŚĆ: pozwala C skoczyć bezpośrednio do ściągania rejestrów
extern interrupt_handler

%macro ISR_NOERRCODE 1
global isr%1
isr%1:
    push dword 0
    push dword %1
    jmp isr_common_stub
%endmacro

%macro ISR_ERRCODE 1
global isr%1
isr%1:
    push dword %1
    jmp isr_common_stub
%endmacro

ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE   8
ISR_NOERRCODE 9
ISR_ERRCODE   10
ISR_ERRCODE   11
ISR_ERRCODE   12
ISR_ERRCODE   13
ISR_ERRCODE   14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_ERRCODE   17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_ERRCODE   30
ISR_NOERRCODE 31

ISR_NOERRCODE 32  ; Timer
ISR_NOERRCODE 33  ; Klawiatura

ISR_NOERRCODE 128 ; syscalls

isr_common_stub:
    pushad

    push ds
    push es
    push fs
    push gs

    mov ax, 0x10    ; Segment danych jądra
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp
    call interrupt_handler
    mov esp, eax    ; Przełączenie stosu na (być może) nowy proces

; --- TUTAJ SKACZEMY PRZY PIERWSZYM URUCHOMIENIU ---
pop_and_iret:
    pop ds
    pop es
    pop fs
    pop gs

    popad
    add esp, 8
    iret
