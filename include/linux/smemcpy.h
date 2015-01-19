#ifndef __LINUX_SMEMCPY_H_
#define __LINUX_SMEMCPY_H_

#include <linux/linkage.h>

extern unsigned memcpy_cycles;

asmlinkage long sys_memcpy(unsigned *val);

#endif
