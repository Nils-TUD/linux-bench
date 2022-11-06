
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>

static int open(struct inode *device_file, struct file *instance) {
    pr_info("tcu device was opened\n");
    return 0;
}

#ifndef __HAVE_PHYS_MEM_ACCESS_PROT

/*
 * Architectures vary in how they handle caching for addresses
 * outside of main memory.
 *
 */
#ifdef pgprot_noncached
static int uncached_access(struct file *file, phys_addr_t addr)
{
#if defined(CONFIG_IA64)
	/*
	 * On ia64, we ignore O_DSYNC because we cannot tolerate memory
	 * attribute aliases.
	 */
	return !(efi_mem_attributes(addr) & EFI_MEMORY_WB);
#elif defined(CONFIG_MIPS)
	{
		extern int __uncached_access(struct file *file,
					     unsigned long addr);

		return __uncached_access(file, addr);
	}
#else
	/*
	 * Accessing memory above the top the kernel knows about or through a
	 * file pointer
	 * that was marked O_DSYNC will be done non-cached.
	 */
	if (file->f_flags & O_DSYNC)
		return 1;
	return addr >= __pa(high_memory);
#endif
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
#endif

static const struct vm_operations_struct mmap_mem_ops = {
#ifdef CONFIG_HAVE_IOREMAP_PROT
	.access = generic_access_phys
#endif
};

static int mmap(struct file *file, struct vm_area_struct *vma) {
	size_t size = vma->vm_end - vma->vm_start;
	phys_addr_t offset = (phys_addr_t)vma->vm_pgoff << PAGE_SHIFT;

	/* Does it even fit in phys_addr_t? */
	if (offset >> PAGE_SHIFT != vma->vm_pgoff)
		return -EINVAL;

	/* It's illegal to wrap around the end of the physical address space. */
	if (offset + (phys_addr_t)size - 1 < offset)
		return -EINVAL;

/*
	if (!valid_mmap_phys_addr_range(vma->vm_pgoff, size))
		return -EINVAL;

	if (!private_mapping_ok(vma))
		return -ENOSYS;

	if (!range_is_allowed(vma->vm_pgoff, size))
		return -EPERM;

	if (!phys_mem_access_prot_allowed(file, vma->vm_pgoff, size,
						&vma->vm_page_prot))
		return -EINVAL;
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

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = open,
    .mmap = mmap,
};


int __init tcu_init(void) {
    const int dev_count = 1;
    static struct class *dev_class;
    int retval;
    struct device *device;
    dev_t dev = 0;
    static struct cdev etx_cdev;

    pr_info("tcu driver init\n");
    retval = alloc_chrdev_region(&dev, 0, dev_count, "tcu");
    if (retval < 0) {
        pr_err("failed to allocate major number for tcu device\n");
        return -1;
    }
    cdev_init(&etx_cdev, &fops);
    retval = cdev_add(&etx_cdev, dev, 1);
    if (retval < 0) {
        pr_err("failed to add tcu device\n");
        unregister_chrdev_region(dev, dev_count);
        return -1;
    }
    dev_class = class_create(THIS_MODULE, "tcu");
    if (IS_ERR(dev_class)) {
        pr_err("failed to create device class for tcu device\n");
        unregister_chrdev_region(dev, dev_count);
        return -1;
    }
    device = device_create(dev_class, NULL, dev, NULL, "tcu");
    if (IS_ERR(device)) {
        pr_err("failed to create tcu device\n");
        unregister_chrdev_region(dev, dev_count);
        class_destroy(dev_class);
        return -1;
    }

    pr_info("created tcu device\n");

    return 0;
}