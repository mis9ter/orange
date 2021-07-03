#pragma once

typedef struct Y_INSTANCE_CONTEXT
{
	ULONG				FileSystemType;		//	FLT_FILESYSTEM_TYPE. FltInstanceSetup �� ����. 
											//	���Ͻý��� Ÿ��. NTFS, FAT, CDFS ��
} Y_INSTANCE_CONTEXT, *PY_INSTANCE_CONTEXT;

typedef struct Y_STREAMHANDLE_CONTEXT
{
	LIST_ENTRY		ListEntry;

	ACCESS_MASK		accessMask;
	ULONG			disposition;
	ULONG			createOption;


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