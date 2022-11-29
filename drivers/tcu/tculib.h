
#include <linux/kernel.h>
#include <linux/io.h>

#define MMIO_PRIV_ADDR 0xf0002000
#define MMIO_PRIV_SIZE (2 * PAGE_SIZE)

// start address of the private tcu mmio region (is mapped to MMIO_PRIV_ADDR)
static uint64_t* priv_base = (uint64_t*)NULL;


// PrivReg
#define PRIV_CMD     0x1
#define PRIV_CMD_ARG 0x2

// PrivCmdOpCode
#define IDLE         0x0
#define INS_TLB      0x3
#define XCHG_ACT     0x4

static void write_priv_reg(unsigned int index, uint64_t val) {
    BUG_ON(priv_base == NULL);
    iowrite64(val, priv_base + index);
}

static uint64_t read_priv_reg(unsigned int index) {
    BUG_ON(priv_base == NULL);
    return ioread64(priv_base + index);
}

static bool check_priv_error(void) {
    uint64_t cmd;
    while (true) {
        cmd = read_priv_reg(PRIV_CMD);
        if ((cmd & 0xf) == IDLE) {
            if (((cmd >> 4) & 0xf) != 0) {
                return true; // error
            }
            return false; // success
        }
    }
}

static void insert_tlb(uint16_t asid, uint64_t virt, uint64_t phys, uint32_t perm) {
    uint64_t cmd;
    write_priv_reg(PRIV_CMD_ARG, phys);
    // TODO: fence
    cmd = ((uint64_t)asid) << 41
        | ((virt & PAGE_MASK) << 9)
        | (perm << 9)
        | INS_TLB;
    write_priv_reg(PRIV_CMD, cmd);
    if (check_priv_error()) {
        pr_err("failed to insert tlb entry\n");
    }
}

static uint64_t xchg_activity(uint64_t actid) {
    write_priv_reg(PRIV_CMD, (actid << 9) | XCHG_ACT);
    if (check_priv_error()) {
        pr_err("failed to exchange activities\n");
    }
    return read_priv_reg(PRIV_CMD_ARG);
}
