
#include "sidecalls.h"
#include "tculib.h"
#include "cfg.h"

// module_init, module_exit
#include <linux/module.h>

// mmap_mem_ops
#include <linux/mm.h>

// cdev_add, cdev_init, cdev
#include <linux/cdev.h>

// ioremap, iounmap
#include <linux/io.h>

// kmalloc
#include <linux/slab.h>

// privileged activity id
#define PRIV_AID 0xffff
// invalid activity id
#define INVAL_AID 0xfffe

typedef struct {
	// current activity id
	// set to PRIV_AID or aid depending on whether we are in privileged or unprivileged mode
	ActId cur_aid;
	// set to activity id as soon as we register an activity
	ActId aid;
} state_t;

static state_t state = { .cur_aid = PRIV_AID, .aid = INVAL_AID };

typedef struct {
	uint64_t virt;
	uint8_t perm;
} TlbInsert;

// register an activity, sets the act id in the state
#define IOCTL_RGSTR_ACT _IOW('q', 1, ActId *)
// inserts an entry in tcu tlb, uses current activity id
#define IOCTL_TLB_INSRT _IOW('q', 4, TlbInsert *)
// forgets about an activity
#define IOCTL_UNREG_ACT _IO('q', 5)

static inline bool in_inval_mode(void)
{
	return state.cur_aid == INVAL_AID;
}

static inline bool in_unpriv_mode(void)
{
	return state.cur_aid != INVAL_AID && state.cur_aid != PRIV_AID;
}

static inline bool in_priv_mode(void)
{
	return state.cur_aid == PRIV_AID;
}

static inline Error switch_to_inval(void) {
	BUG_ON(!in_priv_mode());
	state.aid = INVAL_AID;
	state.cur_aid = INVAL_AID;
	return xchg_activity(state.cur_aid);
}

static inline Error switch_to_unpriv(void) {
	BUG_ON(!in_priv_mode());
	BUG_ON(state.aid == INVAL_AID);
	BUG_ON(state.aid == PRIV_AID);
	state.cur_aid = state.aid;
	return xchg_activity(state.cur_aid);
}

static inline Error switch_to_priv(void) {
	BUG_ON(in_priv_mode());
	state.cur_aid = PRIV_AID;
	return xchg_activity(state.cur_aid);
}

static int ioctl_register_activity(unsigned long arg)
{
	ActId aid;
	Error e;
	BUG_ON(in_priv_mode());
	if (!in_inval_mode()) {
		pr_err("there is already an activity registered");
		return -EINVAL;
	}
	if (copy_from_user(&aid, (ActId *)arg, sizeof(ActId))) {
		return -EACCES;
	}
	state.aid = aid;
	switch_to_priv();
	e = snd_rcv_sidecall_lx_act();
	switch_to_unpriv();
	return (int)e;
}

// source: https://github.com/davidhcefx/Translate-Virtual-Address-To-Physical-Address-in-Linux-Kernel
static unsigned long vaddr2paddr(unsigned long address)
{
	uint64_t phys = 0;
	pgd_t *pgd = pgd_offset(current->mm, address);
	if (!pgd_none(*pgd) && !pgd_bad(*pgd)) {
		p4d_t *p4d = p4d_offset(pgd, address);
		if (!p4d_none(*p4d) && !p4d_bad(*p4d)) {
			pud_t *pud = pud_offset(p4d, address);
			if (!pud_none(*pud) && !pud_bad(*pud)) {
				pmd_t *pmd = pmd_offset(pud, address);
				if (!pmd_none(*pmd) && !pmd_bad(*pmd)) {
					pte_t *pte =
						pte_offset_map(pmd, address);
					if (!pte_none(*pte)) {
						struct page *pg = pte_page(*pte);
						phys = page_to_phys(pg);
					}
					pte_unmap(pte);
				}
			}
		}
	}
	return phys;
}

static int ioctl_insert_tlb(unsigned long arg) {
	TlbInsert ti;
	uint64_t phys;
	BUG_ON(in_priv_mode());
	if (!in_unpriv_mode()) {
		pr_err("there is no activity registered");
	}
	if (copy_from_user(&ti, (TlbInsert *)arg, sizeof(TlbInsert))) {
		return -EACCES;
	}
	// TODO: check that it is mapped for the current process?
	phys = vaddr2paddr(ti.virt);
	if (phys == 0) {
		pr_err("tlb insert: virtual address is not mapped");
		return -EINVAL;
	}
	return (int)insert_tlb(state.cur_aid, ti.virt, phys, ti.perm);
}

static int ioctl_unregister_activity(void)
{
	Error e;
	BUG_ON(in_priv_mode());
	if (!in_unpriv_mode()) {
		pr_err("there is no activity registered");
		return -EINVAL;
	}
	switch_to_priv();
	e = snd_rcv_sidecall_exit(state.aid, 0);
	switch_to_inval();
	return (int)e;
}

static long int tcu_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
	case IOCTL_RGSTR_ACT:
		return ioctl_register_activity(arg);
	case IOCTL_TLB_INSRT:
		return ioctl_insert_tlb(arg);
	case IOCTL_UNREG_ACT:
		return ioctl_unregister_activity();
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
	unpriv_base = (uint64_t *)ioremap(MMIO_ADDR, MMIO_SIZE);
	if (!unpriv_base) {
		dev_t tcu_dev = MKDEV(major, TCU_MINOR);
		pr_err("failed to ioremap the private tcu mmio interface");
		device_destroy(dev_class, tcu_dev);
		unregister_chrdev_region(dev, DEV_COUNT);
		class_destroy(dev_class);
		return -1;
	}
	priv_base = unpriv_base + (MMIO_UNPRIV_SIZE / sizeof(uint64_t));
	rcv_buf = (uint8_t *)memremap(TILEMUX_RBUF_SPACE, KPEX_RBUF_SIZE,
				      MEMREMAP_WB);
	snd_buf = (uint8_t *)kmalloc(MAX_MSG_SIZE, GFP_KERNEL);
	// the message needs to be 16 byte aligned
	BUG_ON(((uintptr_t)snd_buf) % 16 != 0);

	switch_to_inval();

	pr_info("tcu ioctl register act: %#lx", IOCTL_RGSTR_ACT);
	pr_info("tcu ioctl tlb insert: %#lx", IOCTL_TLB_INSRT);
	pr_info("tcu ioctl exit: %#x", IOCTL_UNREG_ACT);

	return 0;
}

static void __exit tcu_exit(void)
{
	dev_t tcu_dev = MKDEV(major, TCU_MINOR);
	kfree(snd_buf);
	iounmap((void *)unpriv_base);
	memunmap(rcv_buf);
	device_destroy(dev_class, tcu_dev);
	class_destroy(dev_class);
	unregister_chrdev_region(tcu_dev, DEV_COUNT);
	pr_info("removed tcu driver");
}

module_init(tcu_init);
module_exit(tcu_exit);

// MODULE_LICENSE("GPL");
// MODULE_DESCRIPTION("Driver for accessing TCU");