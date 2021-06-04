#include <linux/linkage.h>
#include <linux/ptrace.h>

#define MAX_SYSCALL_TIMES   8192

int sysc_pid = 0;
unsigned long sysc_counter = 0;
unsigned int syscall_nos[MAX_SYSCALL_TIMES];
unsigned long enter_times[MAX_SYSCALL_TIMES];
unsigned long leave_times[MAX_SYSCALL_TIMES];

#undef __SYSCALL
#define __SYSCALL(nr, call) [nr] = #call,

const char *sysc_names[__NR_syscalls] = {
    [0 ... __NR_syscalls - 1] = "sys_ni_syscall",
#include <asm/unistd.h>
};

static inline unsigned long _get_cycles(void) {
    unsigned long val;
    __asm__ volatile (
        "rdcycle %0;"
        : "=r" (val) : : "memory"
    );
    return val;
}

asmlinkage void sysc_trace_enter(unsigned int a0) {
    if(current->pid == sysc_pid) {
        syscall_nos[sysc_counter] = a0;
        enter_times[sysc_counter] = _get_cycles();
    }
}

asmlinkage void sysc_trace_exit(void) {
    if(current->pid == sysc_pid) {
        leave_times[sysc_counter] = _get_cycles();
        sysc_counter = (sysc_counter + 1) % MAX_SYSCALL_TIMES;
    }
}

asmlinkage long sys_syscreset(int pid) {
    sysc_pid = pid;
    sysc_counter = MAX_SYSCALL_TIMES - 1;
    return 0;
}

asmlinkage long sys_sysctrace(void) {
    unsigned long i;
    for(i = 0; i < sysc_counter; ++i) {
        pr_notice(" [%3lu] %s (%d) %011lu %011lu\n",
            i, sysc_names[syscall_nos[i]], syscall_nos[i], enter_times[i], leave_times[i]);
    }
    sysc_pid = 0;
    return 0;
}
