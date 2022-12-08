
#ifndef SIDECALLS_H
#define SIDECALLS_H

#include "tculib.h"

#include <linux/types.h>
#include <linux/string.h>
#include <linux/printk.h>

#define SIZE_OF_MSG_HEADER 24

uint8_t* snd_buf = (uint8_t*)NULL;
uint8_t* rcv_buf = (uint8_t*)NULL;

typedef enum {
	Sidecall_EXIT   = 0,
	Sidecall_LX_ACT = 1,
	Sidecall_NOOP   = 2,
} Sidecalls;

typedef struct {
	uint64_t op;
	uint64_t act_sel;
	uint64_t code;
} Exit;

typedef struct {
	uint64_t op;
} LxAct;

typedef struct {
	uint64_t error;
} DefaultReply;

static Error wait_for_reply(void) {
	size_t offset;
	Error e;
	DefaultReply* r;
	BUG_ON(rcv_buf == NULL);
	do {
		offset = fetch_msg(KPEX_REP);
	} while (offset == ~(size_t)0);
	e = ack_msg(KPEX_REP, offset);
	if (e != Error_None) {
		return e;
	}
	r = (DefaultReply*)(rcv_buf + offset + SIZE_OF_MSG_HEADER);
	return (Error)r->error;
}

static Error snd_rcv_sidecall_exit(ActId aid, uint64_t code) {
	Error e;
	Exit msg = {
		.op = Sidecall_EXIT,
		.act_sel = (uint64_t)aid,
		.code = code,
	};
	size_t len = sizeof(Exit);
	BUG_ON(snd_buf == NULL);
	memcpy(snd_buf, &msg, len);
	e = send_aligned(KPEX_SEP, snd_buf, len, 0, KPEX_REP);
	if (e != Error_None) {
		pr_err("exit sidecall failed: %s", error_to_str(e));
	};
	e = wait_for_reply();
	if (e != Error_None) {
		pr_err("exit sidecall wait_for_reply failed: %s", error_to_str(e));
	}
	return e;
}

static Error snd_rcv_sidecall_lx_act(void) {
	Error e;
	LxAct msg = { .op = Sidecall_LX_ACT };
	size_t len = sizeof(LxAct);
	BUG_ON(snd_buf == NULL);
	memcpy(snd_buf, &msg, len);
	e = send_aligned(KPEX_SEP, snd_buf, len, 0, KPEX_REP);
	if (e != Error_None) {
		pr_err("lx act sidecall failed: %s", error_to_str(e));
		return e;
	}
	e = wait_for_reply();
	if (e != Error_None) {
		pr_err("lx act sidecall wait_for_reply failed: %s", error_to_str(e));
	}
	return e;
}

#endif // SIDECALLS_H