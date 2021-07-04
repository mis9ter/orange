#pragma once

typedef struct Y_INSTANCE_CONTEXT
{
	ULONG				FileSystemType;		//	FLT_FILESYSTEM_TYPE. FltInstanceSetup 의 인자. 
											//	파일시스템 타입. NTFS, FAT, CDFS 등
} Y_INSTANCE_CONTEXT, *PY_INSTANCE_CONTEXT;

typedef struct Y_OPERATION {
	volatile	LONG	nCount;
	volatile	LONG64	nBytes;
	volatile	LONG64	nStartOffset;
	volatile	LONG64	nLastOffset;
	volatile	LONG64	nSize;

	LARGE_INTEGER		firstTime;
	LARGE_INTEGER		lastTime;
} Y_OPERATION;
typedef struct Y_STREAMHANDLE_CONTEXT
{
	//LIST_ENTRY		ListEntry;
	HANDLE			PID;
	HANDLE			TID;
	UID				PUID;

	ACCESS_MASK		accessMask;
	ULONG			disposition;
	ULONG			createOption;

	PFLT_FILE_NAME_INFORMATION	pFileNameInfo;
	BOOLEAN			bIsDirectory;
	UINT64			nSize;

	LARGE_INTEGER	createTime;
	Y_OPERATION		read;
	Y_OPERATION		write;
	

} Y_STREAMHANDLE_CONTEXT, *PY_STREAMHANDLE_CONTEXT;
VOID	FilterInstanceContextCleanup(
	__in PFLT_CONTEXT Context,
	__in FLT_CONTEXT_TYPE ContextType
);
VOID	FilterContextCleanup (
	__in PFLT_CONTEXT Context,
	__in FLT_CONTEXT_TYPE ContextType
);
extern	const FLT_CONTEXT_REGISTRATION ContextRegistration[];