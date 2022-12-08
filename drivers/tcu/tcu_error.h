
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
	// SW Errors
	Error_InvArgs,
	Error_ActivityGone,
	Error_OutOfMem,
	Error_NoSuchFile,
	Error_NotSup,
	Error_NoFreeTile,
	Error_InvalidElf,
	Error_NoSpace,
	Error_Exists,
	Error_XfsLink,
	Error_DirNotEmpty,
	Error_IsDir,
	Error_IsNoDir,
	Error_EPInvalid,
	Error_EndOfFile,
	Error_MsgsWaiting,
	Error_UpcallReply,
	Error_CommitFailed,
	Error_NoKernMem,
	Error_NotFound,
	Error_NotRevocable,
	Error_Timeout,
	Error_ReadFailed,
	Error_WriteFailed,
	Error_Utf8Error,
	Error_BadFd,
	Error_SeekPipe,
	// networking
	Error_InvState,
	Error_WouldBlock,
	Error_InProgress,
	Error_AlreadyInProgress,
	Error_NotConnected,
	Error_IsConnected,
	Error_InvChecksum,
	Error_SocketClosed,
	Error_ConnectionFailed,

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
	case Error_InvArgs:
		return "InvArgs";
	case Error_ActivityGone:
		return "ActivityGone";
	case Error_OutOfMem:
		return "OutOfMem";
	case Error_NoSuchFile:
		return "NoSuchFile";
	case Error_NotSup:
		return "NotSup";
	case Error_NoFreeTile:
		return "NoFreeTile";
	case Error_InvalidElf:
		return "InvalidElf";
	case Error_NoSpace:
		return "NoSpace";
	case Error_Exists:
		return "Exists";
	case Error_XfsLink:
		return "XfsLink";
	case Error_DirNotEmpty:
		return "DirNotEmpty";
	case Error_IsDir:
		return "IsDir";
	case Error_IsNoDir:
		return "IsNoDir";
	case Error_EPInvalid:
		return "EPInvalid";
	case Error_EndOfFile:
		return "EndOfFile";
	case Error_MsgsWaiting:
		return "MsgsWaiting";
	case Error_UpcallReply:
		return "UpcallReply";
	case Error_CommitFailed:
		return "CommitFailed";
	case Error_NoKernMem:
		return "NoKernMem";
	case Error_NotFound:
		return "NotFound";
	case Error_NotRevocable:
		return "NotRevocable";
	case Error_Timeout:
		return "Timeout";
	case Error_ReadFailed:
		return "ReadFailed";
	case Error_WriteFailed:
		return "WriteFailed";
	case Error_Utf8Error:
		return "Utf8Error";
	case Error_BadFd:
		return "BadFd";
	case Error_SeekPipe:
		return "SeekPipe";
	case Error_InvState:
		return "InvState";
	case Error_WouldBlock:
		return "WouldBlock";
	case Error_InProgress:
		return "InProgress";
	case Error_AlreadyInProgress:
		return "AlreadyInProgress";
	case Error_NotConnected:
		return "NotConnected";
	case Error_IsConnected:
		return "IsConnected";
	case Error_InvChecksum:
		return "InvChecksum";
	case Error_SocketClosed:
		return "SocketClosed";
	case Error_ConnectionFailed:
		return "ConnectionFailed";
	default:
		return "Unknown Error";
	}
}

#endif // TCU_ERROR_H