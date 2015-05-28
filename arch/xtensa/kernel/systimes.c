#include <linux/cycles.h>
#include "systimes.h"

int sysc_pid = 0;
unsigned sysc_counter = 0;
unsigned syscall_nos[MAX_SYSCALL_TIMES];
unsigned enter_times[MAX_SYSCALL_TIMES];
unsigned leave_times[MAX_SYSCALL_TIMES];

asmlinkage long sys_syscreset(int pid) {
    sysc_pid = pid;
    sysc_counter = MAX_SYSCALL_TIMES - 1;
    return 0;
}

asmlinkage long sys_sysctrace(void) {
    unsigned i;
    for(i = 0; i < sysc_counter; ++i)
        pr_notice(" [%3u] %3u %011u %011u\n", i, syscall_nos[i], enter_times[i], leave_times[i]);
    sysc_pid = 0;
    return 0;
}
