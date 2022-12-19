
#ifndef SIDECALLS_H
#define SIDECALLS_H

#include "tculib.h"

#include <linux/types.h>
#include <linux/string.h>
#include <linux/printk.h>

#define SIZE_OF_MSG_HEADER 24

uint8_t *snd_buf = (uint8_t *)NULL;
uint8_t *rcv_buf = (uint8_t *)NULL;

typedef enum {
	Sidecall_ACT_INIT  = 0x0,
	Sidecall_TRANSLATE = 0x3,
} Sidecalls;

typedef struct {
	uint64_t op;
	uint64_t act_sel;
	uint64_t time_quota;
	uint64_t pt_quota;
	uint64_t eps_start;
} ActInit;

typedef struct {
	uint64_t op;
	uint64_t act_sel;
	uint64_t virt;
	uint64_t perm;
} Translate;

typedef enum {
	Sidecall_EXIT = 0,
	Sidecall_LX_ACT = 1,
	Sidecall_NOOP = 2,
} Calls;

typedef struct {
	uint64_t op;
	uint64_t act_sel;
	uint64_t code;
} Exit;

typedef struct {
	uint64_t op;
} LxAct;

// used for finding out opcode of incoming sidecall
typedef struct {
	uint64_t opcode;
} DefaultRequest;

// used as a reply from the m3 kernel
typedef struct {
	uint64_t error;
} DefaultReply;

// used as a reply to the m3 kernel
typedef struct {
	uint64_t error;
	uint64_t val1;
	uint64_t val2;
} Response;

static Error send_response(Response res, size_t request_offset) {
	size_t len = sizeof(Response);
	BUG_ON(snd_buf == NULL);
	memcpy(snd_buf, &res, len);
	return reply_aligned(TMSIDE_REP, snd_buf, len, request_offset);
}

static ActId check_for_act_init(void)
{
	DefaultRequest *req;
	ActInit *act_init;
	Response res;
	size_t offset = fetch_msg(TMSIDE_REP);
	if (offset == ~(size_t)0) {
		pr_warn("check_for_act_init: didn't receive any sidecalls\n");
		return INVAL_AID;
	}
	req = (DefaultRequest *)(rcv_buf + offset + SIZE_OF_MSG_HEADER);
	if (req->opcode != Sidecall_ACT_INIT) {
		pr_warn("check_for_act_init: received different sidecall: %llu\n", req->opcode);
		return INVAL_AID;
	}
	size_t i;
	for (i = 0; i < 8; ++i) {
		uint64_t* addr = ((uint64_t*)(rcv_buf + offset)) + i;
		pr_info("0x%px: %#llx\n", addr, *addr);
	}
	act_init = (ActInit *)(rcv_buf + offset + SIZE_OF_MSG_HEADER);
	pr_info("received act init with act_sel: %llu\n", act_init->act_sel);
	res = (Response){ .error=0, .val1=0, .val2=0 };
	send_response(res, offset);
	return act_init->act_sel;
}

static Error wait_for_reply(void)
{
	size_t offset;
	Error e;
	DefaultReply *r;
	BUG_ON(rcv_buf == NULL);
	do {
		offset = fetch_msg(KPEX_REP);
	} while (offset == ~(size_t)0);
	e = ack_msg(KPEX_REP, offset);
	if (e != Error_None) {
		return e;
	}
	r = (DefaultReply *)(rcv_buf + offset + SIZE_OF_MSG_HEADER);
	return (Error)r->error;
}

static Error snd_rcv_sidecall_exit(ActId aid, uint64_t code)
{
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
		pr_err("exit sidecall failed: %s\n", error_to_str(e));
	};
	e = wait_for_reply();
	if (e != Error_None) {
		pr_err("exit sidecall wait_for_reply failed: %s\n",
		       error_to_str(e));
	}
	return e;
}

static Error snd_rcv_sidecall_lx_act(void)
{
	Error e;
	LxAct msg = { .op = Sidecall_LX_ACT };
	size_t len = sizeof(LxAct);
	BUG_ON(snd_buf == NULL);
	memcpy(snd_buf, &msg, len);
	e = send_aligned(KPEX_SEP, snd_buf, len, 0, KPEX_REP);
	if (e != Error_None) {
		pr_err("lx act sidecall failed: %s\n", error_to_str(e));
		return e;
	}
	e = wait_for_reply();
	if (e != Error_None) {
		pr_err("lx act sidecall wait_for_reply failed: %s\n",
		       error_to_str(e));
	}
	return e;
}

#endif // SIDECALLS_H