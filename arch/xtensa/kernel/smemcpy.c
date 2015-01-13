#include <linux/smemcpy.h>
#include <asm/uaccess.h>

static void invalidate_line(unsigned long addr) {
    __asm__ volatile (
        "dhi   %0, 0;"
        : : "a"(addr)
    );
}

static void invalidate_buf(char *buf, size_t size) {
    char *p = buf;
    while(p < buf + size) {
        invalidate_line((unsigned long)p);
        p += 32;
    }
}

static unsigned _get_cycles(void) {
    unsigned val;
    __asm__ volatile (
        "rsr    %0, CCOUNT;"
        : "=a" (val) : : "memory"
    );
    return val;
}

unsigned memcpy_cycles = 0;
static char src[4096];
static char dst[4096];

asmlinkage long sys_memcpy(unsigned *val) {
    // unsigned cycles;
    // if(size > sizeof(src))
    //     return -EINVAL;

    // invalidate_buf(src, sizeof(src));
    // invalidate_buf(dst, sizeof(dst));

    // cycles = _get_cycles();
    // memcpy(dst,src,size);
    // put_user(_get_cycles() - cycles, val);
    put_user(memcpy_cycles, val);
    memcpy_cycles = 0;
    return 0;
}
