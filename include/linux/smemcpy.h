#ifndef __LINUX_SMEMCPY_H_
#define __LINUX_SMEMCPY_H_

#include <linux/linkage.h>

extern unsigned long memcpy_cycles;
extern unsigned long memcpy_bytes;

asmlinkage long sys_memcpy(unsigned long *cycles, unsigned long *bytes);

#endif
