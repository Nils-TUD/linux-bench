#include <linux/smemcpy.h>
#include <asm/uaccess.h>

unsigned memcpy_cycles = 0;
unsigned memcpy_bytes = 0;

asmlinkage long sys_memcpy(unsigned *cycles, unsigned *bytes) {
    put_user(memcpy_cycles, cycles);
    if(bytes)
        put_user(memcpy_bytes, bytes);
    memcpy_cycles = 0;
    memcpy_bytes = 0;
    return 0;
}
