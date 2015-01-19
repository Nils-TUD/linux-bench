#ifndef __LINUX_CYCLES_H_
#define __LINUX_CYCLES_H_

#include <linux/linkage.h>

static unsigned _get_cycles(void) {
    unsigned val;
    __asm__ volatile (
        "rsr    %0, CCOUNT;"
        : "=a" (val) : : "memory"
    );
    return val;
}

asmlinkage long sys_get_cycles(unsigned *val);

#endif

