#ifndef __KERNEL_SYS_TIMES_H_
#define __KERNEL_SYS_TIMES_H_

#include <linux/cycles.h>
#include <linux/ptrace.h>

#define MAX_SYSCALL_TIMES   8192

#if defined(__xtensa__)
#   define SYSTIMES_PARMS

// the time we spend for measuring the system call times
#   define OVERHEAD_TIME       (460 - 431)
// the time for entering the kernel (up to _get_cycles()) and leaving it again
#   define MISSING_TIME        (431 - 78)
#else
#   define SYSTIMES_PARMS       __attribute__((regparm(1)))

// TODO verify that numbers
#   define OVERHEAD_TIME       (342 - 320)
#   define MISSING_TIME        (320 - 100)
#endif

extern int sysc_pid;
extern unsigned sysc_counter;
extern unsigned syscall_nos[];
extern unsigned enter_times[];
extern unsigned leave_times[];
extern unsigned overhead_time;

SYSTIMES_PARMS asmlinkage long systimes_enter(unsigned no);
asmlinkage void systimes_leave(void);

asmlinkage long sys_syscreset(int pid);
asmlinkage long sys_sysctrace(void);

#endif
