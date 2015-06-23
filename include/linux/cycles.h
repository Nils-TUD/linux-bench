#ifndef __LINUX_CYCLES_H_
#define __LINUX_CYCLES_H_

#include <linux/linkage.h>

static unsigned long _get_cycles(void) {
#ifdef CONFIG_XTENSA
    unsigned val;
    __asm__ volatile (
        "rsr    %0, CCOUNT;"
        : "=a" (val) : : "memory"
    );
    return val;
#else
    unsigned u,l;
    __asm__ volatile ("rdtsc" : "=a" (l), "=d" (u));
    return (unsigned long)u << 32 | l;
#endif
}

asmlinkage long sys_get_cycles(unsigned long *val);

#endif

