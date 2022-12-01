
#ifndef SIDECALLS_H
#define SIDECALLS_H

#include "tculib.h"

#include <linux/types.h>

// the buffer where sidecalls from kernel arrive
uint8_t* rcv_buf = (uint8_t*)NULL;
// buffer for side calls to the m3 kernel
// needs to have an address that fits into 32 bits
uint8_t* msg_buf = (uint8_t*)NULL;

typedef uint16_t ActId;

typedef enum {
    Calls_EXIT   = 0x0,
    Calls_LX_ACT = 0x1,
    Calls_NOOP   = 0x2,
} Calls;

typedef struct {
    uint64_t op;
} LxAct;

typedef struct {
    uint64_t error;
    uint64_t actid;
} LxActReply;

Error send_receive_lx_act(/* out */ ActId* actid) {
    Error e;
    LxActReply* rpl;
    uint64_t off;

    BUG_ON(rcv_buf == NULL);
    BUG_ON(msg_buf == NULL);
    // send
    ((LxAct*)msg_buf)->op = Calls_LX_ACT;
    e = send_aligned(KPEX_SEP, msg_buf, sizeof(LxAct), 0, KPEX_REP);
    if (e) {
        return e;
    }

    // receive
    do {
        off = fetch_msg(KPEX_REP);
    } while (off != (uint64_t)-1);
    rpl = ((LxActReply*)(rcv_buf + off));
    if (rpl->error) {
        pr_err("LX_ACT sidecall failed: error %s\n", error_to_str((Error)rpl->error));
    }
    *actid = rpl->actid;
    ack_msg(KPEX_REP, off);
    return (Error)rpl->error;
}

#endif // SIDECALLS_H