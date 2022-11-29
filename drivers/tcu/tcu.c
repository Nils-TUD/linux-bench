
#include "tculib.h"

// module_init, module_exit
#include <linux/module.h>

// mmap_mem_ops
#include <linux/mm.h>

// cdev_add, cdev_init, cdev
#include <linux/cdev.h>

// ioremap, iounmap, ioread, iowrite
#include <linux/io.h>


typedef struct {
    uint64_t virt;
    uint64_t phys;
    uint32_t perm;
    uint16_t asid;
} xlate_fault_arg_t;

#define IOCTL_XLATE_FAULT _IOW('q', 1, xlate_fault_arg_t*)

static long int tcu_ioctl(struct file *f, unsigned int cmd, unsigned long arg) {
    xlate_fault_arg_t d;
    switch (cmd) {
        case IOCTL_XLATE_FAULT:
			if (copy_from_user(&d, (xlate_fault_arg_t*)arg, sizeof(xlate_fault_arg_t))) {
				return -EACCES;
			}
            // pr_info("received ioctl xlate fault call; virt: %#llx, phys: %#llx, perm: %#x, asid: %#hx\n", d.virt, d.phys, d.perm, d.asid);
            insert_tlb(d.asid, d.virt, d.phys, d.perm);
            break;
        default:
			return -EINVAL;
    }
    return 0;
}

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
#define MMIO_SIZE (2 * PAGE_SIZE)

static int tcu_dev_mmap(struct file *file, struct vm_area_struct *vma) {
	size_t size = vma->vm_end - vma->vm_start;
    // physical address of the mmio area
	phys_addr_t offset = (phys_addr_t)vma->vm_pgoff << PAGE_SHIFT;
    // pr_info("tcu mmap - start: %#lx, end: %#lx, size: %#lx, offset: %#llx\n", vma->vm_start, vma->vm_end, size, offset);

	/* Does it even fit in phys_addr_t? */
	if (offset >> PAGE_SHIFT != vma->vm_pgoff) {
		return -EINVAL;
    }

	/* It's illegal to wrap around the end of the physical address space. */
	if (offset + (phys_addr_t)size - 1 < offset) {
		return -EINVAL;
    }

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

#define DEV_COUNT 1
#define TCU_MINOR 0
unsigned int major = 0;
static struct cdev cdev;
static struct class *dev_class;

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .mmap = tcu_dev_mmap,
    .unlocked_ioctl = tcu_ioctl,
};

static int __init tcu_init(void) {
    int retval;
    struct device *tcu_device;
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
    priv_base = (uint64_t*)ioremap(MMIO_PRIV_ADDR, MMIO_PRIV_SIZE);
    if (!priv_base) {
        dev_t tcu_dev = MKDEV(major, TCU_MINOR);
        pr_err("failed to ioremap the private tcu mmio interface\n");
        device_destroy(dev_class, tcu_dev);
        unregister_chrdev_region(dev, DEV_COUNT);
        class_destroy(dev_class);
        return -1;
    }
    pr_info("ioctl xlate fault magic number: %#lx\n", IOCTL_XLATE_FAULT);

    return 0;
}

static void __exit tcu_exit(void) {
    dev_t tcu_dev = MKDEV(major, TCU_MINOR);
    iounmap((void*)priv_base);
    device_destroy(dev_class, tcu_dev);
    class_destroy(dev_class);
    unregister_chrdev_region(tcu_dev, DEV_COUNT);
    pr_info("removed tcu driver");
}

module_init(tcu_init);
module_exit(tcu_exit);

// MODULE_LICENSE("GPL");
// MODULE_DESCRIPTION("Driver for accessing TCU");