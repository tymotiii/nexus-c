#ifndef SYSCALLS_H
#define SYSCALLS_H

// Wspólne struktury używane przez kernel.c i handlers.c
struct registers {
    unsigned int gs, fs, es, ds;
    unsigned int edi, esi, ebp, esp, ebx, edx, ecx, eax;
    unsigned int int_no, err_code;
    unsigned int eip, cs, eflags, useresp, ss;
};

#define STATE_FREE     0
#define STATE_READY    1
#define STATE_SLEEP    2
#define STATE_BLOCKED  3

struct task {
    unsigned int esp;
    unsigned int page_dir;
    unsigned int kernel_stack;
    unsigned int sleep_ticks;
    unsigned int state;
    int exit_code;
} __attribute__((packed));

// Zwraca nowy ESP (może być inny przy syscall_sleep)
unsigned int handle_syscall(struct registers *regs, unsigned int current_esp);

// User-space API
void syscall_print(const char *msg);
int  syscall_getpid(void);
void syscall_sleep(int ticks);
void syscall_exit(int exit_code);

#endif
