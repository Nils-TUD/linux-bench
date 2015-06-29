#ifndef __SYS_TIMES_H_
#define __SYS_TIMES_H_

#include <linux/cycles.h>
#include <linux/ptrace.h>

#define MAX_SYSCALL_TIMES   8192

// the time we spend for measuring the system call times
#define OVERHEAD_TIME       (460 - 431)
// the time for entering the kernel (up to _get_cycles()) and leaving it again
#define MISSING_TIME        (431 - 78)

extern int sysc_pid;
extern unsigned sysc_counter;
extern unsigned syscall_nos[];
extern unsigned enter_times[];
extern unsigned leave_times[];
extern unsigned overhead_time;

static inline void systimes_enter(struct pt_regs *regs) {
    if(current->pid == sysc_pid) {
        syscall_nos[sysc_counter] = regs->areg[2];
        enter_times[sysc_counter] = _get_cycles() - overhead_time - (MISSING_TIME / 2);
    }
}

static inline void systimes_leave(struct pt_regs *regs) {
    if(current->pid == sysc_pid) {
        leave_times[sysc_counter] = _get_cycles() - overhead_time + (MISSING_TIME / 2);
        sysc_counter = (sysc_counter + 1) % MAX_SYSCALL_TIMES;
        overhead_time += OVERHEAD_TIME;
    }
}

asmlinkage long sys_syscreset(int pid);
asmlinkage long sys_sysctrace(void);

#endif
