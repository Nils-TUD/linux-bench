
#ifndef TCU_ERROR_H
#define TCU_ERROR_H

#include <linux/stringify.h>

typedef enum Error {
	// success
	Error_None = 0,
	// TCU errors
	Error_NoMEP,
	Error_NoSEP,
	Error_NoREP,
	Error_ForeignEP,
	Error_SendReplyEP,
	Error_RecvGone,
	Error_RecvNoSpace,
	Error_RepliesDisabled,
	Error_OutOfBounds,
	Error_NoCredits,
	Error_NoPerm,
	Error_InvMsgOff,
	Error_TranslationFault,
	Error_Abort,
	Error_UnknownCmd,
	Error_RecvOutOfBounds,
	Error_RecvInvReplyEPs,
	Error_SendInvCreditEp,
	Error_SendInvMsgSize,
	Error_TimeoutMem,
	Error_TimeoutNoC,
	Error_PageBoundary,
	Error_MsgUnaligned,
	Error_TLBMiss,
	Error_TLBFull,
} Error;

char *error_to_str(Error e)
{
	switch (e) {
	case Error_None:
		return "None";
	case Error_NoMEP:
		return "NoMEP";
	case Error_NoSEP:
		return "NoSEP";
	case Error_NoREP:
		return "NoREP";
	case Error_ForeignEP:
		return "ForeignEP";
	case Error_SendReplyEP:
		return "SendReplyEP";
	case Error_RecvGone:
		return "RecvGone";
	case Error_RecvNoSpace:
		return "RecvNoSpace";
	case Error_RepliesDisabled:
		return "RepliesDisabled";
	case Error_OutOfBounds:
		return "OutOfBounds";
	case Error_NoCredits:
		return "NoCredits";
	case Error_NoPerm:
		return "NoPerm";
	case Error_InvMsgOff:
		return "InvMsgOff";
	case Error_TranslationFault:
		return "TranslationFault";
	case Error_Abort:
		return "Abort";
	case Error_UnknownCmd:
		return "UnknownCmd";
	case Error_RecvOutOfBounds:
		return "RecvOutOfBounds";
	case Error_RecvInvReplyEPs:
		return "RecvInvReplyEPs";
	case Error_SendInvCreditEp:
		return "SendInvCreditEp";
	case Error_SendInvMsgSize:
		return "SendInvMsgSize";
	case Error_TimeoutMem:
		return "TimeoutMem";
	case Error_TimeoutNoC:
		return "TimeoutNoC";
	case Error_PageBoundary:
		return "PageBoundary";
	case Error_MsgUnaligned:
		return "MsgUnaligned";
	case Error_TLBMiss:
		return "TLBMiss";
	case Error_TLBFull:
		return "TLBFull";
	default:
		return "Unknown Error";
	}
}

#endif // TCU_ERROR_H