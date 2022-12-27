
#ifndef GLOBADDR_H
#define GLOBADDR_H

#include "tculib.h"
#include "cfg.h"

#include <linux/types.h>

typedef uint64_t Phys;
typedef uint64_t GlobAddr;

// TODO: only on gem5
#define TILE_SHIFT 56

static GlobAddr phys_to_glob(Phys addr)
{
    EpId ep;
    EpInfo info;
    uint64_t offset;
    GlobAddr res;
    BUG_ON(addr > 0xffffffff);
    addr -= MEM_OFFSET;
    ep = (addr >> 30) & 0x3;
    offset = addr & 0x3fffffff;
    info = unpack_mem_ep(ep);
    print_ep_info(ep, info);
    res = ((0x80 + (uint64_t)info.tid) << TILE_SHIFT) | (offset + info.addr);
    pr_info("Translated %#llx to %#llx\n", addr, res);
    return res;
}

#endif // GLOBADDR_H