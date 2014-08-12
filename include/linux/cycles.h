#ifndef __LINUX_CYCLES_H_
#define __LINUX_CYCLES_H_

#include <linux/linkage.h>

asmlinkage long sys_get_cycles(unsigned *val);

#endif

