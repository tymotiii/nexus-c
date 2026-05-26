#include "syscalls.h"

void syscall_print(const char *msg) {
    __asm__ volatile (
        "movl $1, %%eax\n"
        "movl %0, %%ebx\n"
        "int $0x80\n"
        :
        : "g"(msg)
        : "eax", "ebx"
    );
}

int syscall_getpid(void) {
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

void syscall_sleep(int ticks) {
    __asm__ volatile (
        "movl $3, %%eax\n"
        "movl %0, %%ebx\n"
        "int $0x80\n"
        :
        : "g"(ticks)
        : "eax", "ebx"
    );
}

void syscall_exit(int exit_code) {
    __asm__ volatile (
        "movl %0, %%ebx\n"
        "movl $4, %%eax\n"
        "int $0x80"
        : : "r"(exit_code) : "ebx"
    );
}
