﻿#pragma once
#ifndef _NTLPCAPI_H
#define _NTLPCAPI_H

// ignore the warning of 4201
#pragma warning(disable: 4201)

#ifdef _KERNEL_MODE
#include <ntddk.h> 
#include <wdm.h>
#endif
#include <windef.h>


// Local Inter-process Communication

#define PORT_CONNECT 0x0001
#define PORT_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0x1)

typedef short CSHORT;

typedef struct _CLIENT_ID64
{
	ULONGLONG UniqueProcess;
	ULONGLONG UniqueThread;
} CLIENT_ID64, *PCLIENT_ID64;

typedef struct _PORT_MESSAGE
{
	union
	{
		struct
		{
			CSHORT DataLength;
			CSHORT TotalLength;
		} s1;
		ULONG Length;
	} u1;
	union
	{
		struct
		{
			CSHORT Type;
			CSHORT DataInfoOffset;
		} s2;
		ULONG ZeroInit;
	} u2;
	union
	{
		CLIENT_ID ClientId;
		double DoNotUseThisField;
	};
	ULONG MessageId;
	union
	{
		SIZE_T ClientViewSize; // only valid for LPC_CONNECTION_REQUEST messages
		ULONG CallbackId; // only valid for LPC_REQUEST messages
	};
} PORT_MESSAGE, *PPORT_MESSAGE;

typedef struct _PORT_DATA_ENTRY
{
	PVOID Base;
	ULONG Size;
} PORT_DATA_ENTRY, *PPORT_DATA_ENTRY;

typedef struct _PORT_DATA_INFORMATION
{
	ULONG CountDataEntries;
	PORT_DATA_ENTRY DataEntries[1];
} PORT_DATA_INFORMATION, *PPORT_DATA_INFORMATION;

#define LPC_REQUEST 1
#define LPC_REPLY 2
#define LPC_DATAGRAM 3
#define LPC_LOST_REPLY 4
#define LPC_PORT_CLOSED 5
#define LPC_CLIENT_DIED 6
#define LPC_EXCEPTION 7
#define LPC_DEBUG_EVENT 8
#define LPC_ERROR_EVENT 9
#define LPC_CONNECTION_REQUEST 10

#define LPC_KERNELMODE_MESSAGE (CSHORT)0x8000
#define LPC_NO_IMPERSONATE (CSHORT)0x4000

// alpc message attribute
#define ALPC_GET_CONNECT 0x200A
#define ALPC_GET_MESSAGE 0x2001

#define PORT_VALID_OBJECT_ATTRIBUTES OBJ_CASE_INSENSITIVE

#ifdef _WIN64
#define PORT_MAXIMUM_MESSAGE_LENGTH 512
#else
#define PORT_MAXIMUM_MESSAGE_LENGTH 256
#endif

#define LPC_MAX_CONNECTION_INFO_SIZE (16 * sizeof(ULONG_PTR))

#define PORT_TOTAL_MAXIMUM_MESSAGE_LENGTH \
    ((PORT_MAXIMUM_MESSAGE_LENGTH + sizeof(PORT_MESSAGE) + LPC_MAX_CONNECTION_INFO_SIZE + 0xf) & ~0xf)


////
//
// CONSTANTS, 接口属性配置
//
////

// PORT ATTRIBUTE FLAGS
#define ALPC_PORTFLG_NONE 0x0
#define ALPC_PORTFLG_LPCPORT 0x1000 // Only usable in kernel
#define ALPC_PORTFLG_ALLOWIMPERSONATION 0x10000 // Can be set by client to allow server to impersonation this client
#define ALPC_PORTFLG_ALLOW_LPC_REQUESTS 0x20000
#define ALPC_PORTFLG_WAITABLE_PORT 0x40000	// Allow port to be used with synchronization mechanisms like Semaphores
#define ALPC_PORTFLG_SYSTEM_PROCESS 0x100000 // Only usable in kernel
#define ALPC_PORTFLG_ALLOW_DUP_OBJECT 0x80000
#define ALPC_PORTFLG_LRPC_WAKE_POLICY1 0x200000
#define ALPC_PORTFLG_LRPC_WAKE_POLICY2 0x400000
#define ALPC_PORTFLG_LRPC_WAKE_POLICY3 0x800000
#define ALPC_PORTFLG_DIRECT_MESSAGE 0x1000000	// There are 5 queues, Main queue, Direct message queue, Large message queue, Pending queue, Canceled queue... guess this attribute specifies to use the direct message queue instead of main queue


typedef struct _LPC_CLIENT_DIED_MSG
{
	PORT_MESSAGE PortMsg;
	LARGE_INTEGER CreateTime;
} LPC_CLIENT_DIED_MSG, *PLPC_CLIENT_DIED_MSG;

typedef struct _PORT_VIEW
{
	ULONG Length;
	HANDLE SectionHandle;
	ULONG SectionOffset;
	SIZE_T ViewSize;
	PVOID ViewBase;
	PVOID ViewRemoteBase;
} PORT_VIEW, *PPORT_VIEW;

typedef struct _REMOTE_PORT_VIEW
{
	ULONG Length;
	SIZE_T ViewSize;
	PVOID ViewBase;
} REMOTE_PORT_VIEW, *PREMOTE_PORT_VIEW;

// WOW64 definitions

// Except in a small number of special cases, WOW64 programs using the LPC APIs must use the 64-bit versions of the
// PORT_MESSAGE, PORT_VIEW and REMOTE_PORT_VIEW data structures. Note that we take a different approach than the
// official NT headers, which produce 64-bit versions in a 32-bit environment when USE_LPC6432 is defined.

typedef struct _PORT_MESSAGE64
{
	union
	{
		struct
		{
			CSHORT DataLength;
			CSHORT TotalLength;
		} s1;
		ULONG Length;
	} u1;
	union
	{
		struct
		{
			CSHORT Type;
			CSHORT DataInfoOffset;
		} s2;
		ULONG ZeroInit;
	} u2;
	union
	{
		CLIENT_ID64 ClientId;
		double DoNotUseThisField;
	};
	ULONG MessageId;
	union
	{
		ULONGLONG ClientViewSize; // only valid for LPC_CONNECTION_REQUEST messages
		ULONG CallbackId; // only valid for LPC_REQUEST messages
	};
} PORT_MESSAGE64, *PPORT_MESSAGE64;

typedef struct _LPC_CLIENT_DIED_MSG64
{
	PORT_MESSAGE64 PortMsg;
	LARGE_INTEGER CreateTime;
} LPC_CLIENT_DIED_MSG64, *PLPC_CLIENT_DIED_MSG64;

typedef struct _PORT_VIEW64
{
	ULONG Length;
	ULONGLONG SectionHandle;
	ULONG SectionOffset;
	ULONGLONG ViewSize;
	ULONGLONG ViewBase;
	ULONGLONG ViewRemoteBase;
} PORT_VIEW64, *PPORT_VIEW64;

typedef struct _REMOTE_PORT_VIEW64
{
	ULONG Length;
	ULONGLONG ViewSize;
	ULONGLONG ViewBase;
} REMOTE_PORT_VIEW64, *PREMOTE_PORT_VIEW64;

// Port creation

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreatePort(
	_Out_ PHANDLE PortHandle,
	_In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
	_In_ ULONG MaxConnectionInfoLength,
	_In_ ULONG MaxMessageLength,
	_In_opt_ ULONG MaxPoolUsage
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateWaitablePort(
	_Out_ PHANDLE PortHandle,
	_In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
	_In_ ULONG MaxConnectionInfoLength,
	_In_ ULONG MaxMessageLength,
	_In_opt_ ULONG MaxPoolUsage
);

// Port connection (client)

NTSYSCALLAPI
NTSTATUS
NTAPI
NtConnectPort(
	_Out_ PHANDLE PortHandle,
	_In_ PUNICODE_STRING PortName,
	_In_ PSECURITY_QUALITY_OF_SERVICE SecurityQos,
	_Inout_opt_ PPORT_VIEW ClientView,
	_Inout_opt_ PREMOTE_PORT_VIEW ServerView,
	_Out_opt_ PULONG MaxMessageLength,
	_Inout_updates_bytes_to_opt_(*ConnectionInformationLength, *ConnectionInformationLength) PVOID ConnectionInformation,
	_Inout_opt_ PULONG ConnectionInformationLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSecureConnectPort(
	_Out_ PHANDLE PortHandle,
	_In_ PUNICODE_STRING PortName,
	_In_ PSECURITY_QUALITY_OF_SERVICE SecurityQos,
	_Inout_opt_ PPORT_VIEW ClientView,
	_In_opt_ PSID RequiredServerSid,
	_Inout_opt_ PREMOTE_PORT_VIEW ServerView,
	_Out_opt_ PULONG MaxMessageLength,
	_Inout_updates_bytes_to_opt_(*ConnectionInformationLength, *ConnectionInformationLength) PVOID ConnectionInformation,
	_Inout_opt_ PULONG ConnectionInformationLength
);

// Port connection (server)

NTSYSCALLAPI
NTSTATUS
NTAPI
NtListenPort(
	_In_ HANDLE PortHandle,
	_Out_ PPORT_MESSAGE ConnectionRequest
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAcceptConnectPort(
	_Out_ PHANDLE PortHandle,
	_In_opt_ PVOID PortContext,
	_In_ PPORT_MESSAGE ConnectionRequest,
	_In_ BOOLEAN AcceptConnection,
	_Inout_opt_ PPORT_VIEW ServerView,
	_Out_opt_ PREMOTE_PORT_VIEW ClientView
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCompleteConnectPort(
	_In_ HANDLE PortHandle
);

// General

NTSYSCALLAPI
NTSTATUS
NTAPI
NtRequestPort(
	_In_ HANDLE PortHandle,
	_In_ PPORT_MESSAGE RequestMessage
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtRequestWaitReplyPort(
	_In_ HANDLE PortHandle,
	_In_ PPORT_MESSAGE RequestMessage,
	_Out_ PPORT_MESSAGE ReplyMessage
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtReplyPort(
	_In_ HANDLE PortHandle,
	_In_ PPORT_MESSAGE ReplyMessage
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtReplyWaitReplyPort(
	_In_ HANDLE PortHandle,
	_Inout_ PPORT_MESSAGE ReplyMessage
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtReplyWaitReceivePort(
	_In_ HANDLE PortHandle,
	_Out_opt_ PVOID *PortContext,
	_In_opt_ PPORT_MESSAGE ReplyMessage,
	_Out_ PPORT_MESSAGE ReceiveMessage
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtReplyWaitReceivePortEx(
	_In_ HANDLE PortHandle,
	_Out_opt_ PVOID *PortContext,
	_In_opt_ PPORT_MESSAGE ReplyMessage,
	_Out_ PPORT_MESSAGE ReceiveMessage,
	_In_opt_ PLARGE_INTEGER Timeout
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtImpersonateClientOfPort(
	_In_ HANDLE PortHandle,
	_In_ PPORT_MESSAGE Message
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtReadRequestData(
	_In_ HANDLE PortHandle,
	_In_ PPORT_MESSAGE Message,
	_In_ ULONG DataEntryIndex,
	_Out_writes_bytes_to_(BufferSize, *NumberOfBytesRead) PVOID Buffer,
	_In_ SIZE_T BufferSize,
	_Out_opt_ PSIZE_T NumberOfBytesRead
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtWriteRequestData(
	_In_ HANDLE PortHandle,
	_In_ PPORT_MESSAGE Message,
	_In_ ULONG DataEntryIndex,
	_In_reads_bytes_(BufferSize) PVOID Buffer,
	_In_ SIZE_T BufferSize,
	_Out_opt_ PSIZE_T NumberOfBytesWritten
);

typedef enum _PORT_INFORMATION_CLASS
{
	PortBasicInformation,
	PortDumpInformation
} PORT_INFORMATION_CLASS;

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationPort(
	_In_ HANDLE PortHandle,
	_In_ PORT_INFORMATION_CLASS PortInformationClass,
	_Out_writes_bytes_to_(Length, *ReturnLength) PVOID PortInformation,
	_In_ ULONG Length,
	_Out_opt_ PULONG ReturnLength
);

// Asynchronous Local Inter-process Communication

// rev
typedef HANDLE ALPC_HANDLE, *PALPC_HANDLE;

#define ALPC_PORFLG_ALLOW_LPC_REQUESTS 0x20000 // rev
#define ALPC_PORFLG_WAITABLE_PORT 0x40000 // dbg
#define ALPC_PORFLG_SYSTEM_PROCESS 0x100000 // dbg

// symbols
typedef struct _ALPC_PORT_ATTRIBUTES
{
	ULONG Flags;
	SECURITY_QUALITY_OF_SERVICE SecurityQos;
	SIZE_T MaxMessageLength;
	SIZE_T MemoryBandwidth;
	SIZE_T MaxPoolUsage;
	SIZE_T MaxSectionSize;
	SIZE_T MaxViewSize;
	SIZE_T MaxTotalSectionSize;
	ULONG DupObjectTypes;
#ifdef _WIN64
	ULONG Reserved;
#endif
} ALPC_PORT_ATTRIBUTES, *PALPC_PORT_ATTRIBUTES;

// begin_rev
#define ALPC_MESSAGE_SECURITY_ATTRIBUTE 0x80000000
#define ALPC_MESSAGE_VIEW_ATTRIBUTE 0x40000000
#define ALPC_MESSAGE_CONTEXT_ATTRIBUTE 0x20000000
#define ALPC_MESSAGE_HANDLE_ATTRIBUTE 0x10000000
// end_rev

// symbols
typedef struct _ALPC_MESSAGE_ATTRIBUTES
{
	ULONG AllocatedAttributes;
	ULONG ValidAttributes;
} ALPC_MESSAGE_ATTRIBUTES, *PALPC_MESSAGE_ATTRIBUTES;

// symbols
typedef struct _ALPC_COMPLETION_LIST_STATE
{
	union
	{
		struct
		{
			ULONG64 Head : 24;
			ULONG64 Tail : 24;
			ULONG64 ActiveThreadCount : 16;
		} s1;
		ULONG64 Value;
	} u1;
} ALPC_COMPLETION_LIST_STATE, *PALPC_COMPLETION_LIST_STATE;

#define ALPC_COMPLETION_LIST_BUFFER_GRANULARITY_MASK 0x3f // dbg

// symbols
typedef struct DECLSPEC_ALIGN(128) _ALPC_COMPLETION_LIST_HEADER
{
	ULONG64 StartMagic;

	ULONG TotalSize;
	ULONG ListOffset;
	ULONG ListSize;
	ULONG BitmapOffset;
	ULONG BitmapSize;
	ULONG DataOffset;
	ULONG DataSize;
	ULONG AttributeFlags;
	ULONG AttributeSize;

	DECLSPEC_ALIGN(128) ALPC_COMPLETION_LIST_STATE State;
	ULONG LastMessageId;
	ULONG LastCallbackId;
	DECLSPEC_ALIGN(128) ULONG PostCount;
	DECLSPEC_ALIGN(128) ULONG ReturnCount;
	DECLSPEC_ALIGN(128) ULONG LogSequenceNumber;

	// the lock of kernel and user is different
#ifdef _KERNEL_MODE
	DECLSPEC_ALIGN(128) KSPIN_LOCK UserLock;
#else 
	DECLSPEC_ALIGN(128) RTL_SRWLOCK UserLock;
#endif

	ULONG64 EndMagic;
} ALPC_COMPLETION_LIST_HEADER, *PALPC_COMPLETION_LIST_HEADER;

// private
typedef struct _ALPC_CONTEXT_ATTR
{
	PVOID PortContext;
	PVOID MessageContext;
	ULONG Sequence;
	ULONG MessageId;
	ULONG CallbackId;
} ALPC_CONTEXT_ATTR, *PALPC_CONTEXT_ATTR;

// begin_rev
#define ALPC_HANDLEFLG_DUPLICATE_SAME_ACCESS 0x10000
#define ALPC_HANDLEFLG_DUPLICATE_SAME_ATTRIBUTES 0x20000
#define ALPC_HANDLEFLG_DUPLICATE_INHERIT 0x80000
// end_rev

// private
typedef struct _ALPC_HANDLE_ATTR32
{
	ULONG Flags;
	ULONG Reserved0;
	ULONG SameAccess;
	ULONG SameAttributes;
	ULONG Indirect;
	ULONG Inherit;
	ULONG Reserved1;
	ULONG Handle;
	ULONG ObjectType; // ObjectTypeCode, not ObjectTypeIndex
	ULONG DesiredAccess;
	ULONG GrantedAccess;
} ALPC_HANDLE_ATTR32, *PALPC_HANDLE_ATTR32;

// private
typedef struct _ALPC_HANDLE_ATTR
{
	ULONG Flags;
	ULONG Reserved0;
	ULONG SameAccess;
	ULONG SameAttributes;
	ULONG Indirect;
	ULONG Inherit;
	ULONG Reserved1;
	HANDLE Handle;
	PALPC_HANDLE_ATTR32 HandleAttrArray;
	ULONG ObjectType; // ObjectTypeCode, not ObjectTypeIndex
	ULONG HandleCount;
	ACCESS_MASK DesiredAccess;
	ACCESS_MASK GrantedAccess;
} ALPC_HANDLE_ATTR, *PALPC_HANDLE_ATTR;

#define ALPC_SECFLG_CREATE_HANDLE 0x20000 // dbg
#define ALPC_SECFLG_NOSECTIONHANDLE 0x40000
// private
typedef struct _ALPC_SECURITY_ATTR
{
	ULONG Flags;
	PSECURITY_QUALITY_OF_SERVICE QoS;
	ALPC_HANDLE ContextHandle; // dbg
} ALPC_SECURITY_ATTR, *PALPC_SECURITY_ATTR;

// begin_rev
#define ALPC_VIEWFLG_NOT_SECURE 0x40000
// end_rev

// private
typedef struct _ALPC_DATA_VIEW_ATTR
{
	ULONG Flags;
	ALPC_HANDLE SectionHandle;
	PVOID ViewBase; // must be zero on input
	SIZE_T ViewSize;
} ALPC_DATA_VIEW_ATTR, *PALPC_DATA_VIEW_ATTR;

// private
typedef enum _ALPC_PORT_INFORMATION_CLASS
{
	AlpcBasicInformation, // q: out ALPC_BASIC_INFORMATION
	AlpcPortInformation, // s: in ALPC_PORT_ATTRIBUTES
	AlpcAssociateCompletionPortInformation, // s: in ALPC_PORT_ASSOCIATE_COMPLETION_PORT
	AlpcConnectedSIDInformation, // q: in SID
	AlpcServerInformation, // q: inout ALPC_SERVER_INFORMATION
	AlpcMessageZoneInformation, // s: in ALPC_PORT_MESSAGE_ZONE_INFORMATION
	AlpcRegisterCompletionListInformation, // s: in ALPC_PORT_COMPLETION_LIST_INFORMATION
	AlpcUnregisterCompletionListInformation, // s: VOID
	AlpcAdjustCompletionListConcurrencyCountInformation, // s: in ULONG
	AlpcRegisterCallbackInformation, // kernel-mode only
	AlpcCompletionListRundownInformation, // s: VOID
	AlpcWaitForPortReferences
} ALPC_PORT_INFORMATION_CLASS;

// private
typedef struct _ALPC_BASIC_INFORMATION
{
	ULONG Flags;
	ULONG SequenceNo;
	PVOID PortContext;
} ALPC_BASIC_INFORMATION, *PALPC_BASIC_INFORMATION;

// private
typedef struct _ALPC_PORT_ASSOCIATE_COMPLETION_PORT
{
	PVOID CompletionKey;
	HANDLE CompletionPort;
} ALPC_PORT_ASSOCIATE_COMPLETION_PORT, *PALPC_PORT_ASSOCIATE_COMPLETION_PORT;

// private
typedef struct _ALPC_SERVER_INFORMATION
{
	union
	{
		struct
		{
			HANDLE ThreadHandle;
		} In;
		struct
		{
			BOOLEAN ThreadBlocked;
			HANDLE ConnectedProcessId;
			UNICODE_STRING ConnectionPortName;
		} Out;
	};
} ALPC_SERVER_INFORMATION, *PALPC_SERVER_INFORMATION;

// private
typedef struct _ALPC_PORT_MESSAGE_ZONE_INFORMATION
{
	PVOID Buffer;
	ULONG Size;
} ALPC_PORT_MESSAGE_ZONE_INFORMATION, *PALPC_PORT_MESSAGE_ZONE_INFORMATION;

// private
typedef struct _ALPC_PORT_COMPLETION_LIST_INFORMATION
{
	PVOID Buffer; // PALPC_COMPLETION_LIST_HEADER
	ULONG Size;
	ULONG ConcurrencyCount;
	ULONG AttributeFlags;
} ALPC_PORT_COMPLETION_LIST_INFORMATION, *PALPC_PORT_COMPLETION_LIST_INFORMATION;

// private
typedef enum _ALPC_MESSAGE_INFORMATION_CLASS
{
	AlpcMessageSidInformation, // q: out SID
	AlpcMessageTokenModifiedIdInformation,  // q: out LUID
	AlpcMessageDirectStatusInformation,
	AlpcMessageHandleInformation, // ALPC_MESSAGE_HANDLE_INFORMATION
	MaxAlpcMessageInfoClass
} ALPC_MESSAGE_INFORMATION_CLASS, *PALPC_MESSAGE_INFORMATION_CLASS;

typedef struct _ALPC_MESSAGE_HANDLE_INFORMATION
{
	ULONG Index;
	ULONG Flags;
	ULONG Handle;
	ULONG ObjectType;
	ACCESS_MASK GrantedAccess;
} ALPC_MESSAGE_HANDLE_INFORMATION, *PALPC_MESSAGE_HANDLE_INFORMATION;

// begin_private

#if (PHNT_VERSION >= PHNT_VISTA)

// System calls

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcCreatePort(
	_Out_ PHANDLE PortHandle,
	_In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
	_In_opt_ PALPC_PORT_ATTRIBUTES PortAttributes
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcDisconnectPort(
	_In_ HANDLE PortHandle,
	_In_ ULONG Flags
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcQueryInformation(
	_In_opt_ HANDLE PortHandle,
	_In_ ALPC_PORT_INFORMATION_CLASS PortInformationClass,
	_Inout_updates_bytes_to_(Length, *ReturnLength) PVOID PortInformation,
	_In_ ULONG Length,
	_Out_opt_ PULONG ReturnLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcSetInformation(
	_In_ HANDLE PortHandle,
	_In_ ALPC_PORT_INFORMATION_CLASS PortInformationClass,
	_In_reads_bytes_opt_(Length) PVOID PortInformation,
	_In_ ULONG Length
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcCreatePortSection(
	_In_ HANDLE PortHandle,
	_In_ ULONG Flags,
	_In_opt_ HANDLE SectionHandle,
	_In_ SIZE_T SectionSize,
	_Out_ PALPC_HANDLE AlpcSectionHandle,
	_Out_ PSIZE_T ActualSectionSize
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcDeletePortSection(
	_In_ HANDLE PortHandle,
	_Reserved_ ULONG Flags,
	_In_ ALPC_HANDLE SectionHandle
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcCreateResourceReserve(
	_In_ HANDLE PortHandle,
	_Reserved_ ULONG Flags,
	_In_ SIZE_T MessageSize,
	_Out_ PALPC_HANDLE ResourceId
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcDeleteResourceReserve(
	_In_ HANDLE PortHandle,
	_Reserved_ ULONG Flags,
	_In_ ALPC_HANDLE ResourceId
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcCreateSectionView(
	_In_ HANDLE PortHandle,
	_Reserved_ ULONG Flags,
	_Inout_ PALPC_DATA_VIEW_ATTR ViewAttributes
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcDeleteSectionView(
	_In_ HANDLE PortHandle,
	_Reserved_ ULONG Flags,
	_In_ PVOID ViewBase
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcCreateSecurityContext(
	_In_ HANDLE PortHandle,
	_Reserved_ ULONG Flags,
	_Inout_ PALPC_SECURITY_ATTR SecurityAttribute
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcDeleteSecurityContext(
	_In_ HANDLE PortHandle,
	_Reserved_ ULONG Flags,
	_In_ ALPC_HANDLE ContextHandle
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcRevokeSecurityContext(
	_In_ HANDLE PortHandle,
	_Reserved_ ULONG Flags,
	_In_ ALPC_HANDLE ContextHandle
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcQueryInformationMessage(
	_In_ HANDLE PortHandle,
	_In_ PPORT_MESSAGE PortMessage,
	_In_ ALPC_MESSAGE_INFORMATION_CLASS MessageInformationClass,
	_Out_writes_bytes_to_opt_(Length, *ReturnLength) PVOID MessageInformation,
	_In_ ULONG Length,
	_Out_opt_ PULONG ReturnLength
);

#define ALPC_MSGFLG_REPLY_MESSAGE 0x1
#define ALPC_MSGFLG_LPC_MODE 0x2 // ?
#define ALPC_MSGFLG_RELEASE_MESSAGE 0x10000 // dbg
#define ALPC_MSGFLG_SYNC_REQUEST 0x20000 // dbg
#define ALPC_MSGFLG_WAIT_USER_MODE 0x100000
#define ALPC_MSGFLG_WAIT_ALERTABLE 0x200000
#define ALPC_MSGFLG_WOW64_CALL 0x80000000 // dbg

// ALPC Connection FLAGS
/// From: https://recon.cx/2008/a/thomas_garnier/LPC-ALPC-paper.pdf
#define ALPC_SYNC_CONNECTION 0x20000 // Synchronous connection request
#define ALPC_USER_WAIT_MODE 0x100000 // Wait in user mode
#define ALPC_WAIT_IS_ALERTABLE 0x200000 // Wait in alertable mode

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcConnectPort(
	_Out_ PHANDLE PortHandle,
	_In_ PUNICODE_STRING PortName,
	_In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
	_In_opt_ PALPC_PORT_ATTRIBUTES PortAttributes,
	_In_ ULONG Flags,
	_In_opt_ PSID RequiredServerSid,
	_Inout_updates_bytes_to_opt_(*BufferLength, *BufferLength) PPORT_MESSAGE ConnectionMessage,
	_Inout_opt_ PULONG BufferLength,
	_Inout_opt_ PALPC_MESSAGE_ATTRIBUTES OutMessageAttributes,
	_Inout_opt_ PALPC_MESSAGE_ATTRIBUTES InMessageAttributes,
	_In_opt_ PLARGE_INTEGER Timeout
);

#if (PHNT_VERSION >= PHNT_WIN8)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcConnectPortEx(
	_Out_ PHANDLE PortHandle,
	_In_ POBJECT_ATTRIBUTES ConnectionPortObjectAttributes,
	_In_opt_ POBJECT_ATTRIBUTES ClientPortObjectAttributes,
	_In_opt_ PALPC_PORT_ATTRIBUTES PortAttributes,
	_In_ ULONG Flags,
	_In_opt_ PSECURITY_DESCRIPTOR ServerSecurityRequirements,
	_Inout_updates_bytes_to_opt_(*BufferLength, *BufferLength) PPORT_MESSAGE ConnectionMessage,
	_Inout_opt_ PSIZE_T BufferLength,
	_Inout_opt_ PALPC_MESSAGE_ATTRIBUTES OutMessageAttributes,
	_Inout_opt_ PALPC_MESSAGE_ATTRIBUTES InMessageAttributes,
	_In_opt_ PLARGE_INTEGER Timeout
);
#endif

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcAcceptConnectPort(
	_Out_ PHANDLE PortHandle,
	_In_ HANDLE ConnectionPortHandle,
	_In_ ULONG Flags,
	_In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
	_In_opt_ PALPC_PORT_ATTRIBUTES PortAttributes,
	_In_opt_ PVOID PortContext,
	_In_reads_bytes_(ConnectionRequest->u1.s1.TotalLength) PPORT_MESSAGE ConnectionRequest,
	_Inout_opt_ PALPC_MESSAGE_ATTRIBUTES ConnectionMessageAttributes,
	_In_ BOOLEAN AcceptConnection
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcSendWaitReceivePort(
	_In_ HANDLE PortHandle,
	_In_ ULONG Flags,
	_In_reads_bytes_opt_(SendMessage->u1.s1.TotalLength) PPORT_MESSAGE SendMessage,
	_Inout_opt_ PALPC_MESSAGE_ATTRIBUTES SendMessageAttributes,
	_Out_writes_bytes_to_opt_(*BufferLength, *BufferLength) PPORT_MESSAGE ReceiveMessage,
	_Inout_opt_ PSIZE_T BufferLength,
	_Inout_opt_ PALPC_MESSAGE_ATTRIBUTES ReceiveMessageAttributes,
	_In_opt_ PLARGE_INTEGER Timeout
);

#define ALPC_CANCELFLG_TRY_CANCEL 0x1 // dbg
#define ALPC_CANCELFLG_NO_CONTEXT_CHECK 0x8
#define ALPC_CANCELFLGP_FLUSH 0x10000 // dbg

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcCancelMessage(
	_In_ HANDLE PortHandle,
	_In_ ULONG Flags,
	_In_ PALPC_CONTEXT_ATTR MessageContext
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcImpersonateClientOfPort(
	_In_ HANDLE PortHandle,
	_In_ PPORT_MESSAGE Message,
	_In_ PVOID Flags
);

#if (PHNT_VERSION >= PHNT_THRESHOLD)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcImpersonateClientContainerOfPort(
	_In_ HANDLE PortHandle,
	_In_ PPORT_MESSAGE Message,
	_In_ ULONG Flags
);
#endif

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcOpenSenderProcess(
	_Out_ PHANDLE ProcessHandle,
	_In_ HANDLE PortHandle,
	_In_ PPORT_MESSAGE PortMessage,
	_In_ ULONG Flags,
	_In_ ACCESS_MASK DesiredAccess,
	_In_ POBJECT_ATTRIBUTES ObjectAttributes
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlpcOpenSenderThread(
	_Out_ PHANDLE ThreadHandle,
	_In_ HANDLE PortHandle,
	_In_ PPORT_MESSAGE PortMessage,
	_In_ ULONG Flags,
	_In_ ACCESS_MASK DesiredAccess,
	_In_ POBJECT_ATTRIBUTES ObjectAttributes
);

// Support functions

NTSYSAPI
ULONG
NTAPI
AlpcMaxAllowedMessageLength(
	VOID
);

NTSYSAPI
ULONG
NTAPI
AlpcGetHeaderSize(
	_In_ ULONG Flags
);

#define ALPC_ATTRFLG_ALLOCATEDATTR 0x20000000
#define ALPC_ATTRFLG_VALIDATTR 0x40000000
#define ALPC_ATTRFLG_KEEPRUNNINGATTR 0x60000000

NTSYSAPI
NTSTATUS
NTAPI
AlpcInitializeMessageAttribute(
	_In_ ULONG AttributeFlags,
	_Out_opt_ PALPC_MESSAGE_ATTRIBUTES Buffer,
	_In_ ULONG BufferSize,
	_Out_ PULONG RequiredBufferSize
);

NTSYSAPI
PVOID
NTAPI
AlpcGetMessageAttribute(
	_In_ PALPC_MESSAGE_ATTRIBUTES Buffer,
	_In_ ULONG AttributeFlag
);

NTSYSAPI
NTSTATUS
NTAPI
AlpcRegisterCompletionList(
	_In_ HANDLE PortHandle,
	_Out_ PALPC_COMPLETION_LIST_HEADER Buffer,
	_In_ ULONG Size,
	_In_ ULONG ConcurrencyCount,
	_In_ ULONG AttributeFlags
);

NTSYSAPI
NTSTATUS
NTAPI
AlpcUnregisterCompletionList(
	_In_ HANDLE PortHandle
);

#if (PHNT_VERSION >= PHNT_WIN7)
// rev
NTSYSAPI
NTSTATUS
NTAPI
AlpcRundownCompletionList(
	_In_ HANDLE PortHandle
);
#endif

NTSYSAPI
NTSTATUS
NTAPI
AlpcAdjustCompletionListConcurrencyCount(
	_In_ HANDLE PortHandle,
	_In_ ULONG ConcurrencyCount
);

NTSYSAPI
BOOLEAN
NTAPI
AlpcRegisterCompletionListWorkerThread(
	_Inout_ PVOID CompletionList
);

NTSYSAPI
BOOLEAN
NTAPI
AlpcUnregisterCompletionListWorkerThread(
	_Inout_ PVOID CompletionList
);

NTSYSAPI
VOID
NTAPI
AlpcGetCompletionListLastMessageInformation(
	_In_ PVOID CompletionList,
	_Out_ PULONG LastMessageId,
	_Out_ PULONG LastCallbackId
);

NTSYSAPI
ULONG
NTAPI
AlpcGetOutstandingCompletionListMessageCount(
	_In_ PVOID CompletionList
);

NTSYSAPI
PPORT_MESSAGE
NTAPI
AlpcGetMessageFromCompletionList(
	_In_ PVOID CompletionList,
	_Out_opt_ PALPC_MESSAGE_ATTRIBUTES *MessageAttributes
);

NTSYSAPI
VOID
NTAPI
AlpcFreeCompletionListMessage(
	_Inout_ PVOID CompletionList,
	_In_ PPORT_MESSAGE Message
);

NTSYSAPI
PALPC_MESSAGE_ATTRIBUTES
NTAPI
AlpcGetCompletionListMessageAttributes(
	_In_ PVOID CompletionList,
	_In_ PPORT_MESSAGE Message
);

/******************************************声明未文档化方法***********************************************/

// NTSTATUS ZwClose(HANDLE Handle);

NTSYSAPI
NTSTATUS
NTAPI
ZwAlpcCreatePort(
	_Out_ PHANDLE PortHandle,
	_In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
	_In_opt_ PALPC_PORT_ATTRIBUTES PortAttributes
);

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwAlpcSendWaitReceivePort(
	_In_ HANDLE PortHandle,
	_In_ ULONG Flags,
	_In_reads_bytes_opt_(SendMessage->u1.s1.TotalLength) PPORT_MESSAGE SendMessage,
	_Inout_opt_ PALPC_MESSAGE_ATTRIBUTES SendMessageAttributes,
	_Out_writes_bytes_to_opt_(*BufferLength, *BufferLength) PPORT_MESSAGE ReceiveMessage,
	_Inout_opt_ PSIZE_T BufferLength,
	_Inout_opt_ PALPC_MESSAGE_ATTRIBUTES ReceiveMessageAttributes,
	_In_opt_ PLARGE_INTEGER Timeout
);

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwAlpcAcceptConnectPort(
	_Out_ PHANDLE PortHandle,
	_In_ HANDLE ConnectionPortHandle,
	_In_ ULONG Flags,
	_In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
	_In_opt_ PALPC_PORT_ATTRIBUTES PortAttributes,
	_In_opt_ PVOID PortContext,
	_In_reads_bytes_(ConnectionRequest->u1.s1.TotalLength) PPORT_MESSAGE ConnectionRequest,
	_Inout_opt_ PALPC_MESSAGE_ATTRIBUTES ConnectionMessageAttributes,
	_In_ BOOLEAN AcceptConnection
);

NTSYSCALLAPI
NTSTATUS
NTAPI
ZwAlpcConnectPort(
	_Out_ PHANDLE PortHandle,
	_In_ PUNICODE_STRING PortName,
	_In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
	_In_opt_ PALPC_PORT_ATTRIBUTES PortAttributes,
	_In_ ULONG Flags,
	_In_opt_ PSID RequiredServerSid,
	_Inout_updates_bytes_to_opt_(*BufferLength, *BufferLength) PPORT_MESSAGE ConnectionMessage,
	_Inout_opt_ PULONG BufferLength, _Inout_opt_ PALPC_MESSAGE_ATTRIBUTES OutMessageAttributes,
	_Inout_opt_ PALPC_MESSAGE_ATTRIBUTES InMessageAttributes,
	_In_opt_ PLARGE_INTEGER Timeout);

NTSYSCALLAPI
NTSTATUS 
NTAPI 	
ZwAlpcConnectPortEx(
	_Out_ PHANDLE PortHandle,
	_In_ POBJECT_ATTRIBUTES ConnectionPortObjectAttributes, 
	_In_opt_ POBJECT_ATTRIBUTES ClientPortObjectAttributes,
	_In_opt_ PALPC_PORT_ATTRIBUTES PortAttributes, 
	_In_ ULONG Flags, 
	_In_opt_ PSECURITY_DESCRIPTOR ServerSecurityRequirements, 
	_Inout_updates_bytes_to_opt_(*BufferLength, *BufferLength) PPORT_MESSAGE ConnectionMessage, 
	_Inout_opt_ PSIZE_T BufferLength, 
	_Inout_opt_ PALPC_MESSAGE_ATTRIBUTES OutMessageAttributes, 
	_Inout_opt_ PALPC_MESSAGE_ATTRIBUTES InMessageAttributes,
	_In_opt_ PLARGE_INTEGER Timeout);

NTSYSAPI
NTSTATUS
NTAPI
ZwAlpcDisconnectPort(
	_In_ HANDLE PortHandle,
	_In_ ULONG Flags
);

/*********************************************进程相关API******************************************************/
#ifdef _KERNEL_MODE
typedef enum _SYSTEM_INFORMATION_CLASS {
	SystemBasicInformation = 0,
	SystemPerformanceInformation = 2,
	SystemTimeOfDayInformation = 3,
	SystemProcessInformation = 5,
	SystemProcessorPerformanceInformation = 8,
	SystemInterruptInformation = 23,
	SystemExceptionInformation = 33,
	SystemRegistryQuotaInformation = 37,
	SystemLookasideInformation = 45,
	SystemPolicyInformation = 134,
} SYSTEM_INFORMATION_CLASS;

typedef struct _SYSTEM_PROCESS_INFORMATION {
	ULONG           NextEntryOffset;
	ULONG           NumberOfThreads;
	LARGE_INTEGER   Reserved[3];
	LARGE_INTEGER   CreateTime;
	LARGE_INTEGER   UserTime;
	LARGE_INTEGER   KernelTime;
	UNICODE_STRING  ImageName;
	KPRIORITY       BasePriority;
	HANDLE          ProcessId;
	HANDLE          InheritedFromProcessId;
	ULONG           HandleCount;
	UCHAR           Reserved4[4];
	PVOID           Reserved5[11];
	SIZE_T          PeakPagefileUsage;
	SIZE_T          PrivatePageCount;
	LARGE_INTEGER   Reserved6[6];
} SYSTEM_PROCESS_INFORMATION, * PSYSTEM_PROCESS_INFORMATION;

typedef struct _LDR_DATA_TABLE_ENTRY {
	LIST_ENTRY     LoadOrder;
	LIST_ENTRY     MemoryOrder;
	LIST_ENTRY     InitializationOrder;
	PVOID          ModuleBaseAddress;
	PVOID          EntryPoint;
	ULONG          ModuleSize;
	UNICODE_STRING FullModuleName;
	UNICODE_STRING ModuleName;
	ULONG          Flags;
	USHORT         LoadCount;
	USHORT         TlsIndex;
	union {
		LIST_ENTRY Hash;
		struct {
			PVOID SectionPointer;
			ULONG CheckSum;
		} s;
	} u;
	ULONG   TimeStamp;
} LDR_DATA_TABLE_ENTRY, * PLDR_DATA_TABLE_ENTRY;

NTSYSAPI NTSTATUS NTAPI ZwQuerySystemInformation(
	_In_      SYSTEM_INFORMATION_CLASS SystemInformationClass,
	_Inout_   PVOID                    SystemInformation,
	_In_      ULONG                    SystemInformationLength,
	_Out_opt_ PULONG                   ReturnLength
);

#define PROCESS_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0xFFFF)
#define PROCESS_CREATE_PROCESS 0x0080
#define PROCESS_CREATE_THREAD 0x0002
#define PROCESS_DUP_HANDLE 0x0040	
#define PROCESS_QUERY_INFORMATION 0x0400
#define PROCESS_QUERY_LIMITED_INFORMATION 0x1000
#define PROCESS_SET_INFORMATION 0x0200
#define PROCESS_SET_QUOTA 0x0100
#define PROCESS_SUSPEND_RESUME 0x0800
#define PROCESS_TERMINATE 0x0001
#define PROCESS_VM_OPERATION 0x0008
#define PROCESS_VM_READ 0x0010
#define PROCESS_VM_WRITE 0x0020
#define SYNCHRONIZE 0x001000000L

NTSTATUS WINAPI ZwQueryInformationProcess(
	_In_ HANDLE ProcessHandle,
	_In_ PROCESSINFOCLASS ProcessInformationClass,
	_Out_ PVOID ProcessInformation,
	_In_ ULONG ProcessInformationLength,
	_Out_opt_ PULONG ReturnLength
);

#define EXHANDLE_TABLE_ENTRY_LOCK_BIT    1

// 有这个结构，先添加着，后续根据情况处理
typedef struct _EXHANDLE
{
	union
	{
		struct
		{
			ULONG TagBits : 2;
			ULONG Index : 30;
		};
		VOID* GenericHandleOverlay;
		ULONG Value;
	};
} EXHANDLE, * PEXHANDLE;


// 查阅网上资料，多是展示的下一个多一个参数的版本，先使用来看看
//typedef struct _HANDLE_TABLE_ENTRY {
//	union {
//		VOID* Object;
//		ULONG_PTR Value;
//	} u1;
//	union {
//		ULONG GrantedAccess;
//		ULONG_PTR NextFreeTableEntry;
//	} u2;
//} HANDLE_TABLE_ENTRY, * PHANDLE_TABLE_ENTRY;

typedef struct _HANDLE_TABLE_ENTRY
{
	union
	{
		PVOID Object;
		ULONG ObAttributes;
		ULONG_PTR Value;
	} u1;
	union
	{
		ACCESS_MASK GrantedAccess;
		LONG NextFreeTableEntry;
	} u2;
} HANDLE_TABLE_ENTRY, * PHANDLE_TABLE_ENTRY;

typedef struct _HANDLE_TABLE_WIN8 {
	ULONG NextHandleNeedingPool;
	LONG ExtraInfoPages;
	volatile ULONG TableCode;
	struct _EPROCESS* QuotaProcess;
	struct _LIST_ENTRY HandleTableList;
	ULONG UniqueProcessId;
	ULONG Flags;
	EX_PUSH_LOCK HandleContentionEvent;
	EX_PUSH_LOCK HandleTableLock;
	// ... other useless fields
} HANDLE_TABLE_WIN8, * PHANDLE_TABLE_WIN8;

// Windows 7
typedef BOOLEAN(*EX_ENUMERATE_HANDLE_ROUTINE)(
	IN PHANDLE_TABLE_ENTRY HandleTableEntry,
	IN HANDLE Handle,
	IN PVOID EnumParameter
	);

// Windows 8
typedef BOOLEAN(*EX_ENUMERATE_HANDLE_ROUTINE_WIN8)(
	IN PVOID PspCidTable,
	IN PHANDLE_TABLE_ENTRY HandleTableEntry,
	IN HANDLE Handle,
	IN PVOID EnumParameter
	);

NTKERNELAPI
BOOLEAN
ExEnumHandleTable(
	_In_  PVOID HandleTable,
	_In_  EX_ENUMERATE_HANDLE_ROUTINE EnumHandleProcedure,
	_In_  PVOID EnumParameter,
	_Out_opt_ PHANDLE Handle
);

NTKERNELAPI
VOID
FASTCALL
ExfUnblockPushLock(
	PEX_PUSH_LOCK PushLock,
	PVOID CurrentWaitBlock
);
#endif

/***********************************************************************************************************/

#ifdef _KERNEL_MODE
// 声明系统api，防止与头文件重定义了
NTKERNELAPI
UCHAR*
PsGetProcessImageFileName(
	__in PEPROCESS Process
);

NTKERNELAPI
NTSTATUS
PsLookupProcessByProcessId(
	_In_ HANDLE ProcessId,
	_Out_ PEPROCESS* Process
);

NTKERNELAPI
HANDLE
PsGetCurrentProcessId(
	VOID
);
#endif

/******************************************定义为函数指针类型***********************************************/
typedef NTSTATUS(*NtAlpcCreatePort_FuncType)(
	_Out_ PHANDLE PortHandle,
	_In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
	_In_opt_ PALPC_PORT_ATTRIBUTES PortAttributes
	);

typedef NTSTATUS(*NtAlpcDisconnectPort_FuncType)(
	_In_ HANDLE PortHandle,
	_In_ ULONG Flags
	);

typedef NTSTATUS(*NtAlpcQueryInformation_FuncType)(
	_In_opt_ HANDLE PortHandle,
	_In_ ALPC_PORT_INFORMATION_CLASS PortInformationClass,
	_Inout_updates_bytes_to_(Length, *ReturnLength) PVOID PortInformation,
	_In_ ULONG Length,
	_Out_opt_ PULONG ReturnLength
	);

typedef NTSTATUS(*NtAlpcSetInformation_FuncType)(
	_In_ HANDLE PortHandle,
	_In_ ALPC_PORT_INFORMATION_CLASS PortInformationClass,
	_In_reads_bytes_opt_(Length) PVOID PortInformation,
	_In_ ULONG Length
	);

typedef NTSTATUS(*NtAlpcCreatePortSection_FuncType)(
	_In_ HANDLE PortHandle,
	_In_ ULONG Flags,
	_In_opt_ HANDLE SectionHandle,
	_In_ SIZE_T SectionSize,
	_Out_ PALPC_HANDLE AlpcSectionHandle,
	_Out_ PSIZE_T ActualSectionSize
	);

typedef NTSTATUS(*NtAlpcDeletePortSection_FuncType)(
	_In_ HANDLE PortHandle,
	_Reserved_ ULONG Flags,
	_In_ ALPC_HANDLE SectionHandle
	);

typedef NTSTATUS(*NtAlpcCreateResourceReserve_FuncType)(
	_In_ HANDLE PortHandle,
	_Reserved_ ULONG Flags,
	_In_ SIZE_T MessageSize,
	_Out_ PALPC_HANDLE ResourceId
	);

typedef NTSTATUS(*NtAlpcDeleteResourceReserve_FuncType)(
	_In_ HANDLE PortHandle,
	_Reserved_ ULONG Flags,
	_In_ ALPC_HANDLE ResourceId
	);

typedef NTSTATUS(*NtAlpcCreateSectionView_FuncType)(
	_In_ HANDLE PortHandle,
	_Reserved_ ULONG Flags,
	_Inout_ PALPC_DATA_VIEW_ATTR ViewAttributes
	);

typedef NTSTATUS(*NtAlpcDeleteSectionView_FuncType)(
	_In_ HANDLE PortHandle,
	_Reserved_ ULONG Flags,
	_In_ PVOID ViewBase
	);

typedef NTSTATUS(*NtAlpcCreateSecurityContext_FuncType)(
	_In_ HANDLE PortHandle,
	_Reserved_ ULONG Flags,
	_Inout_ PALPC_SECURITY_ATTR SecurityAttribute
	);

typedef NTSTATUS(*NtAlpcDeleteSecurityContext_FuncType)(
	_In_ HANDLE PortHandle,
	_Reserved_ ULONG Flags,
	_In_ ALPC_HANDLE ContextHandle
	);

typedef NTSTATUS(*NtAlpcRevokeSecurityContext_FuncType)(
	_In_ HANDLE PortHandle,
	_Reserved_ ULONG Flags,
	_In_ ALPC_HANDLE ContextHandle
	);

typedef NTSTATUS(*NtAlpcQueryInformationMessage_FuncType)(
	_In_ HANDLE PortHandle,
	_In_ PPORT_MESSAGE PortMessage,
	_In_ ALPC_MESSAGE_INFORMATION_CLASS MessageInformationClass,
	_Out_writes_bytes_to_opt_(Length, *ReturnLength) PVOID MessageInformation,
	_In_ ULONG Length,
	_Out_opt_ PULONG ReturnLength
	);

typedef NTSTATUS(*NtAlpcConnectPort_FuncType)(
	_Out_ PHANDLE PortHandle,
	_In_ PUNICODE_STRING PortName,
	_In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
	_In_opt_ PALPC_PORT_ATTRIBUTES PortAttributes,
	_In_ ULONG Flags,
	_In_opt_ PSID RequiredServerSid,
	_Inout_updates_bytes_to_opt_(*BufferLength, *BufferLength) PPORT_MESSAGE ConnectionMessage,
	_Inout_opt_ PULONG BufferLength,
	_Inout_opt_ PALPC_MESSAGE_ATTRIBUTES OutMessageAttributes,
	_Inout_opt_ PALPC_MESSAGE_ATTRIBUTES InMessageAttributes,
	_In_opt_ PLARGE_INTEGER Timeout
	);

typedef NTSTATUS(*NtAlpcAcceptConnectPort_FuncType)(
	_Out_ PHANDLE PortHandle,
	_In_ HANDLE ConnectionPortHandle,
	_In_ ULONG Flags,
	_In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
	_In_opt_ PALPC_PORT_ATTRIBUTES PortAttributes,
	_In_opt_ PVOID PortContext,
	_In_reads_bytes_(ConnectionRequest->u1.s1.TotalLength) PPORT_MESSAGE ConnectionRequest,
	_Inout_opt_ PALPC_MESSAGE_ATTRIBUTES ConnectionMessageAttributes,
	_In_ BOOLEAN AcceptConnection
	);

typedef NTSTATUS(*NtAlpcSendWaitReceivePort_FuncType)(
	_In_ HANDLE PortHandle,
	_In_ ULONG Flags,
	_In_reads_bytes_opt_(SendMessage->u1.s1.TotalLength) PPORT_MESSAGE SendMessage,
	_Inout_opt_ PALPC_MESSAGE_ATTRIBUTES SendMessageAttributes,
	_Out_writes_bytes_to_opt_(*BufferLength, *BufferLength) PPORT_MESSAGE ReceiveMessage,
	_Inout_opt_ PSIZE_T BufferLength,
	_Inout_opt_ PALPC_MESSAGE_ATTRIBUTES ReceiveMessageAttributes,
	_In_opt_ PLARGE_INTEGER Timeout
	);

typedef NTSTATUS(*NtAlpcCancelMessage_FuncType)(
	_In_ HANDLE PortHandle,
	_In_ ULONG Flags,
	_In_ PALPC_CONTEXT_ATTR MessageContext
	);

typedef NTSTATUS(*NtAlpcImpersonateClientOfPort_FuncType)(
	_In_ HANDLE PortHandle,
	_In_ PPORT_MESSAGE Message,
	_In_ PVOID Flags
	);

typedef NTSTATUS(*NtAlpcOpenSenderProcess_FuncType)(
	_Out_ PHANDLE ProcessHandle,
	_In_ HANDLE PortHandle,
	_In_ PPORT_MESSAGE PortMessage,
	_In_ ULONG Flags,
	_In_ ACCESS_MASK DesiredAccess,
	_In_ POBJECT_ATTRIBUTES ObjectAttributes
	);

typedef NTSTATUS(*NtAlpcOpenSenderThread_FuncType)(
	_Out_ PHANDLE ThreadHandle,
	_In_ HANDLE PortHandle,
	_In_ PPORT_MESSAGE PortMessage,
	_In_ ULONG Flags,
	_In_ ACCESS_MASK DesiredAccess,
	_In_ POBJECT_ATTRIBUTES ObjectAttributes
	);

typedef ULONG(*AlpcMaxAllowedMessageLength_FuncType)(VOID);

typedef ULONG(*AlpcGetHeaderSize_FuncType)(_In_ ULONG Flags);

typedef NTSTATUS(*AlpcInitializeMessageAttribute_FuncType)(
	_In_ ULONG AttributeFlags,
	_Out_opt_ PALPC_MESSAGE_ATTRIBUTES Buffer,
	_In_ ULONG BufferSize,
	_Out_ PULONG RequiredBufferSize
	);

typedef PVOID(*AlpcGetMessageAttribute_FuncType)(
	_In_ PALPC_MESSAGE_ATTRIBUTES Buffer,
	_In_ ULONG AttributeFlag
	);

typedef NTSTATUS(*AlpcRegisterCompletionList_FuncType)(
	_In_ HANDLE PortHandle,
	_Out_ PALPC_COMPLETION_LIST_HEADER Buffer,
	_In_ ULONG Size,
	_In_ ULONG ConcurrencyCount,
	_In_ ULONG AttributeFlags
	);

typedef NTSTATUS(*AlpcUnregisterCompletionList_FuncType)(_In_ HANDLE PortHandle);

typedef NTSTATUS(*AlpcAdjustCompletionListConcurrencyCount_FuncType)(
	_In_ HANDLE PortHandle,
	_In_ ULONG ConcurrencyCount
	);

typedef BOOLEAN(*AlpcRegisterCompletionListWorkerThread_FuncType)(_Inout_ PVOID CompletionList);

typedef BOOLEAN(*AlpcUnregisterCompletionListWorkerThread_FuncType)(_Inout_ PVOID CompletionList);

typedef VOID(*AlpcGetCompletionListLastMessageInformation_FuncType)(
	_In_ PVOID CompletionList,
	_Out_ PULONG LastMessageId,
	_Out_ PULONG LastCallbackId
	);

typedef ULONG(*AlpcGetOutstandingCompletionListMessageCount_FuncType)(_In_ PVOID CompletionList);

typedef PPORT_MESSAGE(*AlpcGetMessageFromCompletionList_FuncType)(
	_In_ PVOID CompletionList,
	_Out_opt_ PALPC_MESSAGE_ATTRIBUTES* MessageAttributes
	);

typedef VOID(*AlpcFreeCompletionListMessage_FuncType)(
	_Inout_ PVOID CompletionList,
	_In_ PPORT_MESSAGE Message
	);

typedef PALPC_MESSAGE_ATTRIBUTES(*AlpcGetCompletionListMessageAttributes_FuncType)(
	_In_ PVOID CompletionList,
	_In_ PPORT_MESSAGE Message
	);

// 以下为应用层使用

#ifndef _KERNEL_MODE
NTSYSAPI
PVOID
NTAPI
RtlAllocateHeap(
	_In_ PVOID HeapHandle,
	_In_opt_ ULONG Flags,
	_In_ SIZE_T Size
);

NTSYSAPI
BOOLEAN
NTAPI
RtlFreeHeap(
	_In_ PVOID HeapHandle,
	_In_opt_ ULONG Flags,
	_In_ PVOID BaseAddress
);

typedef PVOID(NTAPI* RtlAllocateHeap_FuncType)(
	_In_ PVOID HeapHandle,
	_In_opt_ ULONG Flags,
	_In_ SIZE_T Size
	);

typedef BOOLEAN(NTAPI* RtlFreeHeap_FuncType)(
	_In_ PVOID HeapHandle,
	_In_opt_ ULONG Flags,
	_In_ PVOID BaseAddress
	);

typedef BOOLEAN(NTAPI* RtlCreateUnicodeString_FuncType)(
	PUNICODE_STRING DestinationString,
	PCWSTR SourceString
	);

typedef VOID(NTAPI* RtlFreeUnicodeString_FuncType)(
	PUNICODE_STRING UnicodeString
	);


typedef VOID(*RtlInitUnicodeString_FuncType)(
	PUNICODE_STRING DestinationString,
	PCWSTR SourceString
	);

typedef VOID (*RtlCopyUnicodeString_FuncType)(
	_Out_ PUNICODE_STRING DestinationString,
	_In_opt_ PCUNICODE_STRING SourceString
);

typedef NTSTATUS (*RtlAppendUnicodeToString_FuncType)(
	_In_opt_ PUNICODE_STRING Destination,
	_In_opt_ PCWSTR          Source
);

typedef NTSTATUS(NTAPI* NtQueryInformationProcess_FuncType)(
	_In_ HANDLE ProcessHandle,
	_In_ int ProcessInformationClass,
	// _In_ PROCESSINFOCLASS ProcessInformationClass,
	_Out_writes_bytes_(ProcessInformationLength) PVOID ProcessInformation,
	_In_ ULONG ProcessInformationLength,
	_Out_opt_ PULONG ReturnLength
	);

typedef NTSTATUS(NTAPI* NtReadVirtualMemory_FuncType)(
	HANDLE ProcessHandle,
	PVOID BaseAddress,
	PVOID Buffer,
	SIZE_T BufferSize,
	PSIZE_T NumberOfBytesRead
	);

typedef NTSTATUS(NTAPI* RtlGetVersion_FuncType)(
	PRTL_OSVERSIONINFOW lpVersionInformation
	);

#endif
/***********************************************************************************************************/

#endif

// end_private
#ifndef MAX_MSG_LEN
#define MAX_MSG_LEN 0x1000
#endif

#ifdef _KERNEL_MODE

#ifndef ALPCTAG
#define ALPCTAG 'cpla'
#endif

static PVOID GetAPIAddress(PCWSTR funcname) {
	UNICODE_STRING functionName;
	RtlInitUnicodeString(&functionName, funcname);
	return MmGetSystemRoutineAddress(&functionName);
}

#define CALLAPI(func) \
        ((func##_FuncType)GetAPIAddress(L#func))

#define GETAPINORTN(funcname, funcaddress)      \
    funcname##_FuncType funcaddress =                \
        (funcname##_FuncType)GetAPIAddress(L#funcname); \
    if (funcaddress == NULL) {                       \
        return;                             \
    }

#define GETAPIRTN(funcname, funcaddress, retValue)      \
    funcname##_FuncType funcaddress =                \
        (funcname##_FuncType)GetAPIAddress(L#funcname); \
    if (funcaddress == NULL) {                       \
        return retValue;                             \
    }


#else

static PVOID GetAPIAddress(LPCSTR funcname) {
	PVOID res = NULL;
	HMODULE hntdll = GetModuleHandleA("ntdll.dll");
	if (hntdll) {
		res = GetProcAddress(hntdll, funcname);
	}
	return res;
}

#define DEFAPI(func) \
		func##_FuncType pfunc_##func = (func##_FuncType)(GetAPIAddress(#func))

#endif


#endif
