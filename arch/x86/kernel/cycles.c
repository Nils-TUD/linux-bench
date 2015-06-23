#include <linux/cycles.h>
#include <asm/uaccess.h>

asmlinkage long sys_get_cycles(unsigned long *val) {
    unsigned long cycles = _get_cycles();
    put_user(cycles, val);
    return 0;
}

