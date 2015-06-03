#ifndef __LINUX_SMEMCPY_H_
#define __LINUX_SMEMCPY_H_

#include <linux/linkage.h>

extern unsigned memcpy_cycles;
extern unsigned memcpy_bytes;

asmlinkage long sys_memcpy(unsigned *cycles, unsigned *bytes);

#endif
