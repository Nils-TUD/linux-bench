#include <linux/smemcpy.h>
#include <asm/uaccess.h>

unsigned memcpy_cycles = 0;

asmlinkage long sys_memcpy(unsigned *val) {
    put_user(memcpy_cycles, val);
    memcpy_cycles = 0;
    return 0;
}
