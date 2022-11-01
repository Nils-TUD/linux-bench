
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>

int __init tcu_init(void) {
    printk(KERN_INFO "TCU driver init\n");
    return 0;
}