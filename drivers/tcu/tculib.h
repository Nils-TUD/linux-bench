
#ifndef TCULIB_H
#define TCULIB_H

#include "tcu_error.h"

#include <linux/kernel.h>
#include <linux/io.h>
#include <asm/barrier.h>

typedef uint64_t EpId;
typedef uint32_t Label;
typedef uint64_t Reg;

#define MMIO_ADDR 0xf0000000
#define MMIO_UNPRIV_SIZE (2 * PAGE_SIZE)
#define MMIO_SIZE (4 * PAGE_SIZE)

#define PMEM_PROT_EPS 4
/// The send EP for kernel calls from TileMux
#define KPEX_SEP ((EpId)PMEM_PROT_EPS + 0)
/// The receive EP for kernel calls from TileMux
#define KPEX_REP ((EpId)PMEM_PROT_EPS + 1)


/// The number of external registers
#define EXT_REGS 2

#define MAX_MSG_SIZE 512

// start address of the private tcu mmio region (is mapped to MMIO_PRIV_ADDR)
static Reg* priv_base = (Reg*)NULL;
static Reg* unpriv_base = (Reg*)NULL;

typedef enum PrivReg {
    /// For core requests
    PrivReg_CORE_REQ      = 0x0,
    /// For privileged commands
    PrivReg_PRIV_CMD      = 0x1,
    /// The argument for privileged commands
    PrivReg_PRIV_CMD_ARG  = 0x2,
    /// The current activity
    PrivReg_CUR_ACT       = 0x3,
    /// Used to ack IRQ requests
    PrivReg_CLEAR_IRQ     = 0x4,
} PrivReg;
#define PRIV_CMD_ARG 0x2

typedef enum {
    /// The idle command has no effect
    PrivCmdOpCode_IDLE        = 0,
    /// Invalidate a single TLB entry
    PrivCmdOpCode_INV_PAGE    = 1,
    /// Invalidate all TLB entries
    PrivCmdOpCode_INV_TLB     = 2,
    /// Insert an entry into the TLB
    PrivCmdOpCode_INS_TLB     = 3,
    /// Changes the activity
    PrivCmdOpCode_XCHG_ACT    = 4,
    /// Sets the timer
    PrivCmdOpCode_SET_TIMER   = 5,
    /// Abort the current command
    PrivCmdOpCode_ABORT_CMD   = 6,
    /// Flushes and invalidates the cache
    PrivCmdOpCode_FLUSH_CACHE = 7,
} PrivCmdOpCode;


typedef enum {
    /// The idle command has no effect
    CmdOpCode_IDLE          = 0x0,
    /// Sends a message
    CmdOpCode_SEND          = 0x1,
    /// Replies to a message
    CmdOpCode_REPLY         = 0x2,
    /// Reads from external memory
    CmdOpCode_READ          = 0x3,
    /// Writes to external memory
    CmdOpCode_WRITE         = 0x4,
    /// Fetches a message
    CmdOpCode_FETCH_MSG     = 0x5,
    /// Acknowledges a message
    CmdOpCode_ACK_MSG       = 0x6,
    /// Puts the CU to sleep
    CmdOpCode_SLEEP         = 0x7,
} CmdOpCode;

typedef enum {
    /// Starts commands and signals their completion
    UnprivReg_COMMAND  = 0x0,
    /// Specifies the data address and size
    UnprivReg_DATA     = 0x1,
    /// Specifies an additional argument
    UnprivReg_ARG1     = 0x2,
    /// The current time in nanoseconds
    UnprivReg_CUR_TIME = 0x3,
    /// Prints a line into the gem5 log
    UnprivReg_PRINT    = 0x4,
} UnprivReg;

static void write_priv_reg(unsigned int index, Reg val) {
    BUG_ON(priv_base == NULL);
    pr_info("write_priv_reg index: %u, addr: 0x%px, val: %#llx", index, priv_base + index, val);
    iowrite64(val, priv_base + index);
}

static Reg read_priv_reg(unsigned int index) {
    BUG_ON(priv_base == NULL);
    return ioread64(priv_base + index);
}

static void write_unpriv_reg(unsigned int index, Reg val) {
    BUG_ON(unpriv_base == NULL);
    pr_info("write_unpriv_reg index: %u, addr: 0x%px, val: %#llx", index, unpriv_base + index, val);
    iowrite64(val, unpriv_base + EXT_REGS + index);
}

static Reg read_unpriv_reg(unsigned int index) {
    BUG_ON(unpriv_base == NULL);
    return ioread64(unpriv_base + EXT_REGS + index);
}

static Error get_priv_error(void) {
    Reg cmd;
    while (true) {
        cmd = read_priv_reg(PrivReg_PRIV_CMD);
        if ((cmd & 0xf) == PrivCmdOpCode_IDLE) {
            return (Error)((cmd >> 4) & 0xf);
        }
    }
}

static Reg build_data(Reg addr, Reg size) {
    BUG_ON(addr >> 32 != 0);
    return (size << 32) | addr;
}

static Reg build_cmd(EpId ep, CmdOpCode cmd, Reg arg) {
    return (arg << 25) | (ep << 4) | cmd;
}

static Error get_error(void) {
    while (true) {
        Reg cmd = read_unpriv_reg(UnprivReg_COMMAND);
        if ((cmd & 0xf) == CmdOpCode_IDLE) {
            return (Error)((cmd >> 20) & 0x1f);
        }
    }
}

static Error perform_send_reply(size_t msg_addr, Reg cmd) {
    write_unpriv_reg(UnprivReg_COMMAND, cmd);
    return get_error();
}

static Error send_aligned(EpId ep, uint8_t* msg, size_t len, Label reply_lbl, EpId reply_ep) {
    Error e;
    write_unpriv_reg(UnprivReg_DATA, build_data((Reg)msg, len));
    if (reply_lbl != 0) {
        write_unpriv_reg(UnprivReg_ARG1, reply_lbl);
    }
    e = perform_send_reply((size_t)msg, build_cmd(ep, CmdOpCode_SEND, reply_ep));
    if (e) {
        pr_err("failed to send_aligned, got error %s\n", error_to_str(e));
    }
    return e;
}

// returns (uint64_t)-1 on error
static uint64_t fetch_msg(EpId ep) {
    Error e;
    write_unpriv_reg(UnprivReg_COMMAND, build_cmd(ep, CmdOpCode_FETCH_MSG, 0));
    e = get_error();
    if (e) {
        return (uint64_t)-1;
    }
    return read_unpriv_reg(UnprivReg_ARG1);
}

static Error ack_msg(EpId ep, size_t msg_off) {
    mb();
    write_unpriv_reg(UnprivReg_COMMAND, build_cmd(ep, CmdOpCode_ACK_MSG, (Reg)msg_off));
    return get_error();
}

static Error insert_tlb(uint16_t asid, uint64_t virt, uint64_t phys, uint32_t perm) {
    Reg cmd;
    Error e;
    write_priv_reg(PrivReg_PRIV_CMD_ARG, phys);
    mb();
    cmd = ((Reg)asid << 41)
        | ((virt & PAGE_MASK) << 9)
        | (perm << 9)
        | PrivCmdOpCode_INS_TLB;
    write_priv_reg(PrivReg_PRIV_CMD, cmd);
    e = get_priv_error();
    if (e) {
        pr_err("failed to insert tlb entry, got error %s\n", error_to_str(e));
    }
    return e;
}

static Error xchg_activity(Reg actid) {
    Error e;
    write_priv_reg(PrivReg_PRIV_CMD, (actid << 9) | PrivCmdOpCode_XCHG_ACT);
    e = get_priv_error();
    if (e) {
        pr_err("failed to exchange activities, got error: %s\n", error_to_str(e));
    }
    // read_priv_reg(PrivReg_PRIV_CMD_ARG);
    return e;
}

#endif // TCULIB_H