#include <linux/cycles.h>
#include <asm/uaccess.h>

static unsigned _get_cycles(void) {
	unsigned val;
	__asm__ volatile (
		"rsr    %0, CCOUNT;"
		: "=a" (val) : : "memory"
	);
	return val;
}

asmlinkage long sys_get_cycles(unsigned *val) {
	unsigned cycles = _get_cycles();
	put_user(cycles, val);
	return 0;
}

