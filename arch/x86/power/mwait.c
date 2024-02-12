#include <linux/syscalls.h>
#include <asm/mwait.h>

unsigned long long gem5_debug(unsigned long long msg) {
    unsigned long long res = 0;
    __asm__ volatile(
        ".byte 0x0F, 0x04;"
        ".word 0x63;"
        : "=a"(res)
        : "D"(msg)
    );
    return res;
}

SYSCALL_DEFINE2(mwait, void *, counter, int, user) {
    if(!user) {
        gem5_debug(0xDEAD);

        // pr_info("counter=%px, user=%d\n", counter, user);

        // this is not a correct usage of monitor&mwait, but we don't care about the event here, but
        // only want to measure the latency to put the core to sleep and wakeup again.
        __monitor(counter, 0, 0);
	    asm volatile(".byte 0x0f, 0x01, 0xc9;"
		     :: "a" (0), "c" (0));

        gem5_debug(0xBEEF);
    }
    else
        gem5_debug(0xAFFE);
    return 0;
}
