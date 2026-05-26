#ifndef USRAPI_H
#define USRAPI_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// --- TUTAJ DAJESZ BEZPOŚREDNIO KOD SWOICH SYSCALLI ---

static inline void syscall_print(const char *msg) {
    __asm__ volatile (
        "movl $1, %%eax\n"
        "movl %0, %%ebx\n"
        "int $0x80\n"
        :
        : "g"(msg)
        : "eax", "ebx"
    );
}

static inline int syscall_getpid(void) {
    int retval;
    __asm__ volatile (
        "movl $2, %%eax\n"
        "int $0x80\n"
        "movl %%eax, %0\n"
        : "=r"(retval)
        :
        : "eax"
    );
    return retval;
}

static inline void syscall_sleep(int ticks) {
    __asm__ volatile (
        "movl $3, %%eax\n"
        "movl %0, %%ebx\n"
        "int $0x80\n"
        :
        : "g"(ticks)
        : "eax", "ebx"
    );
}

static inline void syscall_exit(int exit_code) {
    __asm__ volatile (
        "movl %0, %%ebx\n"
        "movl $4, %%eax\n"
        "int $0x80"
        : : "r"(exit_code) : "ebx"
    );
}

static inline char syscall_read() {
    char c;
    __asm__ volatile (
        "int $0x80"          // Wywołanie przerwania syscalla jądra
        : "=a"(c)            // OUTPUT: weź wartość z rejestru EAX i zapisz do zmiennej 'c'
        : "a"(2)             // INPUT: wpisz numer syscalla (2 = SYS_READ) do rejestru EAX
        : "memory"           // Poinformuj kompilator, że pamięć mogła ulec zmianie
    );
    return c;
}


#ifdef __cplusplus
}
#endif

#endif
