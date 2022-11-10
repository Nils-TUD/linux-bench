
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>

#ifdef __HAVE_PHYS_MEM_ACCESS_PROT
#error "__HAVE_PHYS_MEM_ACCESS_PROT is defined"
#endif

#ifdef pgprot_noncached
static int uncached_access(struct file *file, phys_addr_t addr)
{
	if (file->f_flags & O_DSYNC)
		return 1;
	return addr >= __pa(high_memory);
}
#endif

static pgprot_t phys_mem_access_prot(struct file *file, unsigned long pfn,
				     unsigned long size, pgprot_t vma_prot)
{
#ifdef pgprot_noncached
	phys_addr_t offset = pfn << PAGE_SHIFT;

	if (uncached_access(file, offset))
		return pgprot_noncached(vma_prot);
#endif
	return vma_prot;
}

static const struct vm_operations_struct mmap_mem_ops = {
#ifdef CONFIG_HAVE_IOREMAP_PROT
	.access = generic_access_phys
#endif
};

#define MMIO_ADDR 0xf0000000
#define MMIO_SIZE (4 * PAGE_SIZE)

static int tcu_dev_mmap(struct file *file, struct vm_area_struct *vma) {
	size_t size = vma->vm_end - vma->vm_start;
    // physical address of the mmio area
	phys_addr_t offset = (phys_addr_t)vma->vm_pgoff << PAGE_SHIFT;
    pr_info("tcu mmap - start: 0x%lx, end: 0x%lx, size: %lu, offset: 0x%llx\n", vma->vm_start, vma->vm_end, size, offset);

	/* Does it even fit in phys_addr_t? */
	if (offset >> PAGE_SHIFT != vma->vm_pgoff) {
		return -EINVAL;
    }

	/* It's illegal to wrap around the end of the physical address space. */
	if (offset + (phys_addr_t)size - 1 < offset) {
		return -EINVAL;
    }

    // TODO: later, size should only be 2 * PAGE_SIZE
    /* We only want to support mapping the tcu mmio area */
    if (offset != MMIO_ADDR || size != MMIO_SIZE) {
        pr_err("tcu mmap invalid area\n");
        return -EINVAL;
    }

	vma->vm_page_prot = phys_mem_access_prot(file, vma->vm_pgoff,
						 size,
						 vma->vm_page_prot);

	vma->vm_ops = &mmap_mem_ops;

	/* Remap-pfn-range will mark the range VM_IO */
	if (io_remap_pfn_range(vma,
			    vma->vm_start,
			    vma->vm_pgoff,
			    size,
			    vma->vm_page_prot)) {
        pr_err("tcu mmap - remap_pfn_range failed");
		return -EAGAIN;
	}
	return 0;
}

static int msg_dev_mmap(struct file *file, struct vm_area_struct *vma) {
	size_t size = vma->vm_end - vma->vm_start;
    // physical address of the mmio area
	phys_addr_t offset = (phys_addr_t)vma->vm_pgoff << PAGE_SHIFT;
    pr_info("tcu msg mmap - start: 0x%lx, end: 0x%lx, size: %lu, offset: 0x%llx\n", vma->vm_start, vma->vm_end, size, offset);

	/* Does it even fit in phys_addr_t? */
	if (offset >> PAGE_SHIFT != vma->vm_pgoff) {
		return -EINVAL;
    }

	/* It's illegal to wrap around the end of the physical address space. */
	if (offset + (phys_addr_t)size - 1 < offset) {
		return -EINVAL;
    }

    /*
    unsigned long mmio_end = MMIO_ADDR + MMIO_SIZE;
    if ((vma->vm_start <= MMIO_ADDR && vma->vm_end >= MMIO_ADDR)
     || (vma->vm_start <= mmio_end && vma->vm_end >= mmio_end)) {
        pr_err("tcu msg mmap invalid area\n");
        return -EINVAL;
    }
    */

	vma->vm_page_prot = phys_mem_access_prot(file, vma->vm_pgoff,
						 size,
						 vma->vm_page_prot);

	vma->vm_ops = &mmap_mem_ops;

	/* Remap-pfn-range will mark the range VM_IO */
	if (remap_pfn_range(vma,
			    vma->vm_start,
			    vma->vm_pgoff,
			    size,
			    vma->vm_page_prot)) {
		return -EAGAIN;
	}
	return 0;
}

// one device for tcu mmio area mapping and one for the message buffer
// (which needs virtual addresses < 0x1_0000_0000)
#define DEV_COUNT 2
#define TCU_MINOR 0
#define MSG_MINOR 1
unsigned int major = 0;
static struct cdev cdev;
static struct class *dev_class;

static struct file_operations tcu_dev_fops = {
    .owner = THIS_MODULE,
    .mmap = tcu_dev_mmap,
};

static struct file_operations msg_dev_fops = {
    .owner = THIS_MODULE,
    .mmap = msg_dev_mmap,
};

static int open(struct inode *inode, struct file *filp) {
    int minor;

    minor = iminor(inode);
    pr_info("tcu device with minor %d was opened\n", minor);
    if (minor == TCU_MINOR) {
        filp->f_op = &tcu_dev_fops;
    } else if (minor == MSG_MINOR) {
        filp->f_op = &msg_dev_fops;
    }
    return 0;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = open,
};

static int __init tcu_init(void) {
    int retval;
    struct device *tcu_device;
    struct device *msg_device;
    dev_t dev = 0;

    pr_info("tcu driver init\n");
    retval = alloc_chrdev_region(&dev, TCU_MINOR, DEV_COUNT, "tcu");
    if (retval < 0) {
        pr_err("failed to allocate major number for tcu device\n");
        return -1;
    }
    major = MAJOR(dev);
    cdev_init(&cdev, &fops);
    retval = cdev_add(&cdev, dev, DEV_COUNT);
    if (retval < 0) {
        pr_err("failed to add tcu device\n");
        unregister_chrdev_region(dev, DEV_COUNT);
        return -1;
    }
    dev_class = class_create(THIS_MODULE, "tcu");
    if (IS_ERR(dev_class)) {
        pr_err("failed to create device class for tcu device\n");
        unregister_chrdev_region(dev, DEV_COUNT);
        return -1;
    }
    tcu_device = device_create(dev_class, NULL, MKDEV(major, TCU_MINOR), NULL, "tcu");
    if (IS_ERR(tcu_device)) {
        pr_err("failed to create tcu device\n");
        unregister_chrdev_region(dev, DEV_COUNT);
        class_destroy(dev_class);
        return -1;
    }
    msg_device = device_create(dev_class,  NULL, MKDEV(major, MSG_MINOR), NULL, "tcumsg");
    if (IS_ERR(msg_device)) {
        pr_err("failed to create tcu msg device\n");
        device_destroy(dev_class, dev);
        unregister_chrdev_region(dev, DEV_COUNT);
        class_destroy(dev_class);
    }
    pr_info("created tcu and tcumsg devices\n");

    return 0;
}

static void __exit tcu_exit(void) {
    dev_t tcu_dev = MKDEV(major, TCU_MINOR);
    dev_t msg_dev = MKDEV(major, MSG_MINOR);
    device_destroy(dev_class, tcu_dev);
    device_destroy(dev_class, msg_dev);
    class_destroy(dev_class);
    unregister_chrdev_region(tcu_dev, DEV_COUNT);
    pr_info("removed tcu driver");
}

module_init(tcu_init);
module_exit(tcu_exit);

// MODULE_LICENSE("GPL");
// MODULE_DESCRIPTION("Driver for accessing TCU");