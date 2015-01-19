#include <linux/cycles.h>
#include <asm/uaccess.h>

asmlinkage long sys_get_cycles(unsigned *val) {
	unsigned cycles = _get_cycles();
	put_user(cycles, val);
	return 0;
}

