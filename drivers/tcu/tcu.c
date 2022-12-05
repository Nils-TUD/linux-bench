
#include "tculib.h"

// module_init, module_exit
#include <linux/module.h>

// mmap_mem_ops
#include <linux/mm.h>

// cdev_add, cdev_init, cdev
#include <linux/cdev.h>

// ioremap, iounmap
#include <linux/io.h>

// tilemux activity id
#define TMAID 0xffff
#define INVAL_AID 0xfffe
// read permission for tlb entry
#define READ_PERM 0x1

typedef struct {
	// current activity id
	// set to TMAID or aid depending on whether we are in tilemux mode or user mode
	ActId cur_aid;
	// set to activity id as soon as we register an activity
	ActId aid;
} state_t;

static state_t state = { .cur_aid = TMAID, .aid = INVAL_AID };

typedef struct {
	uint64_t phys;
	uint32_t virt;
} tlb_insert_t;

// register an activity, sets the act id in the state
#define IOCTL_RGSTR_ACT _IOW('q', 1, ActId *)
// goes to tilemux mode by setting the act tcu register
#define IOCTL_TO_TMX_MD _IO('q', 2)
// goes to user mode by setting the act tcu register
#define IOCTL_TO_USR_MD _IO('q', 3)
// inserts an entry in tcu tlb, uses current activity id
#define IOCTL_TLB_INSRT _IOW('q', 4, tlb_insert_t *)
// forgets about an activity
#define IOCTL_UNREG_ACT _IO('q', 5)

bool in_tmmode(void)
{
	return state.cur_aid == TMAID;
}

Error set_act_tcu(void)
{
	return xchg_activity(state.cur_aid);
}

static long int tcu_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
	tlb_insert_t ti;
	switch (cmd) {
	case IOCTL_RGSTR_ACT:
		if (copy_from_user(&state.aid, (ActId *)arg, sizeof(ActId))) {
			return -EACCES;
		}
		break;
	case IOCTL_TO_TMX_MD:
		if (in_tmmode()) {
			return 0;
		}
		state.cur_aid = TMAID;
		return (int)set_act_tcu();
	case IOCTL_TO_USR_MD:
		if (!in_tmmode()) {
			return 0;
		}
		if (state.aid == INVAL_AID) {
			return -EPERM;
		}
		state.cur_aid = state.aid;
		return (int)set_act_tcu();
	case IOCTL_TLB_INSRT:
		if (copy_from_user(&ti, (tlb_insert_t *)arg,
				   sizeof(tlb_insert_t))) {
			return -EACCES;
		}
		return (int)insert_tlb(state.cur_aid, ti.virt, ti.phys,
				       READ_PERM);
		break;
	case IOCTL_UNREG_ACT:
		state.cur_aid = TMAID;
		state.aid = INVAL_AID;
		return (int)set_act_tcu();
	default:
		return -EINVAL;
	}
	return 0;
}

#ifdef __HAVE_PHYS_MEM_ACCESS_PROT
#error "__HAVE_PHYS_MEM_ACCESS_PROT is defined"
#endif

// TODO: is this necessary?
#ifdef pgprot_noncached
static int uncached_access(struct file *file, phys_addr_t addr)
{
	if (file->f_flags & O_DSYNC)
		return 1;
	return addr >= __pa(high_memory);
}
#endif

// TODO: is this necessary?
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

// TODO: is this necessary?
static const struct vm_operations_struct mmap_mem_ops = {
#ifdef CONFIG_HAVE_IOREMAP_PROT
	.access = generic_access_phys
#endif
};

static int tcu_dev_mmap(struct file *file, struct vm_area_struct *vma)
{
	size_t size = vma->vm_end - vma->vm_start;
	// physical address of the mmio area
	phys_addr_t offset = (phys_addr_t)vma->vm_pgoff << PAGE_SHIFT;

	/* Does it even fit in phys_addr_t? */
	if (offset >> PAGE_SHIFT != vma->vm_pgoff) {
		return -EINVAL;
	}

	/* It's illegal to wrap around the end of the physical address space. */
	if (offset + (phys_addr_t)size - 1 < offset) {
		return -EINVAL;
	}

	/* We only want to support mapping the tcu mmio area */
	if (offset != MMIO_UNPRIV_ADDR || size != MMIO_UNPRIV_SIZE) {
		pr_err("tcu mmap invalid area");
		return -EINVAL;
	}

	vma->vm_page_prot = phys_mem_access_prot(file, vma->vm_pgoff, size,
						 vma->vm_page_prot);

	vma->vm_ops = &mmap_mem_ops;

	/* Remap-pfn-range will mark the range VM_IO */
	if (io_remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, size,
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

static dev_t create_tcu_dev(void)
{
	int retval;
	struct device *tcu_device;
	dev_t dev = 0;

	retval = alloc_chrdev_region(&dev, TCU_MINOR, DEV_COUNT, "tcu");
	if (retval < 0) {
		pr_err("failed to allocate major number for tcu device");
		return -1;
	}
	major = MAJOR(dev);
	cdev_init(&cdev, &fops);
	retval = cdev_add(&cdev, dev, DEV_COUNT);
	if (retval < 0) {
		pr_err("failed to add tcu device");
		unregister_chrdev_region(dev, DEV_COUNT);
		return -1;
	}
	dev_class = class_create(THIS_MODULE, "tcu");
	if (IS_ERR(dev_class)) {
		pr_err("failed to create device class for tcu device");
		unregister_chrdev_region(dev, DEV_COUNT);
		return -1;
	}
	tcu_device = device_create(dev_class, NULL, MKDEV(major, TCU_MINOR),
				   NULL, "tcu");
	if (IS_ERR(tcu_device)) {
		pr_err("failed to create tcu device");
		unregister_chrdev_region(dev, DEV_COUNT);
		class_destroy(dev_class);
		return -1;
	}
	return dev;
}

static int __init tcu_init(void)
{
	dev_t dev;
	dev = create_tcu_dev();
	if (dev == (dev_t)-1) {
		return -1;
	}
	priv_base = (uint64_t *)ioremap(MMIO_PRIV_ADDR, MMIO_PRIV_SIZE);
	if (!priv_base) {
		dev_t tcu_dev = MKDEV(major, TCU_MINOR);
		pr_err("failed to ioremap the private tcu mmio interface");
		device_destroy(dev_class, tcu_dev);
		unregister_chrdev_region(dev, DEV_COUNT);
		class_destroy(dev_class);
		return -1;
	}
	pr_info("tcu ioctl register act: %#lx", IOCTL_RGSTR_ACT);
	pr_info("tcu ioctl to tilemux mode: %#x", IOCTL_TO_TMX_MD);
	pr_info("tcu ioctl to user mode: %#x", IOCTL_TO_USR_MD);
	pr_info("tcu ioctl tlb insert: %#lx", IOCTL_TLB_INSRT);
	pr_info("tcu ioctl exit: %#x", IOCTL_UNREG_ACT);

	return 0;
}

static void __exit tcu_exit(void)
{
	dev_t tcu_dev = MKDEV(major, TCU_MINOR);
	iounmap((void *)priv_base);
	device_destroy(dev_class, tcu_dev);
	class_destroy(dev_class);
	unregister_chrdev_region(tcu_dev, DEV_COUNT);
	pr_info("removed tcu driver");
}

module_init(tcu_init);
module_exit(tcu_exit);

// MODULE_LICENSE("GPL");
// MODULE_DESCRIPTION("Driver for accessing TCU");