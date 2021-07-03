#include "yfilter.h"


typedef struct Y_FILE_CONTEXT {
	LARGE_INTEGER		times[2];
	HANDLE				PID;
	HANDLE				TID;
} Y_FILE_CONTEXT, *PY_FILE_CONTEXT;

PY_FILE_CONTEXT		CreateFileContext() {
	PY_FILE_CONTEXT		p	= (PY_FILE_CONTEXT)CMemory::Allocate(PagedPool, sizeof(Y_FILE_CONTEXT), 'elif');
	if( p ) {
		KeQuerySystemTime(&p->times[0]);
		//	ExSystemTimeToLocalTime
		p->times[1].QuadPart	= 0;
		p->PID					= PsGetCurrentProcessId();
		p->TID					= PsGetCurrentThreadId();	
	}
	return p;
}
void				ReleaseFileContext(PY_FILE_CONTEXT p) {
	if( p )	CMemory::Free(p);
}
inline bool	IsNegligibleIO(PETHREAD CONST Thread)
{
	//	무시 가능한 녀석들
	//	필터링 중지거나 커널 쓰레드인 경우 체크하지 않음 
	//	(추후 커널레벨에서의 방어가 필요한 경우 IoIsSystemThread 처리 부분 제거)
	if (
		NULL == Thread						||
		NULL == Config()					||
		NULL == Config()->pFilter			||
		IoIsSystemThread(Thread)			||		//	시스템 스레드인 경우
		KernelMode == ExGetPreviousMode()	||		//	The ExGetPreviousMode routine returns the previous processor mode for the current thread.
		KeGetCurrentIrql() != PASSIVE_LEVEL ||
		IoGetTopLevelIrp() != NULL
		)
	{
		//__log("Negligible IO");
		return true;
	}
	return false;
}
inline bool	IsWriteIO(
	ACCESS_MASK     accessMask,
	ULONG           disposition,
	ULONG			createOption
)
{
	if( disposition != FILE_OPEN )	return true;
	if ((accessMask & FILE_WRITE_DATA) || 
		(accessMask & FILE_WRITE_ATTRIBUTES) || 
		(accessMask & FILE_WRITE_EA) || 
		(accessMask & FILE_APPEND_DATA) ||
		(accessMask & WRITE_OWNER) || 
		(accessMask & WRITE_DAC) || 
		(accessMask & DELETE) 
		)
		return true;
	if( createOption & FILE_DELETE_ON_CLOSE)
		return true;
	return false;
}
PFLT_FILE_NAME_INFORMATION	FltGetFileName(
	PFLT_CALLBACK_DATA		pCallbackData,
	PCFLT_RELATED_OBJECTS	pFltObjects,
	bool *					pbIsDirectory
)
{
	/*
	FltGetFileNameInformation
	https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/fltkernel/nf-fltkernel-fltgetfilenameinformation

	FLT_FILE_NAME_NORMALIZED
	The FileNameInformation parameter receives the address of a structure containing the normalized name for the file.

	FLT_FILE_NAME_OPENED
	The FileNameInformation parameter receives the address of a structure containing the name
	that was used when the file was opened.

	FLT_FILE_NAME_SHORT
	The FileNameInformation parameter receives the address of a structure containing the short (8.3) name for the file.
	The short name consists of up to 8 characters, followed immediately by a period and up to 3 more characters.
	The short name for a file does not include the volume name, directory path, or stream name.
	Not valid in the pre-create path.

	FLT_FILE_NAME_QUERY_DEFAULT
	If it is not currently safe to query the file system for the file name, FltGetFileNameInformation does nothing.
	Otherwise, FltGetFileNameInformation queries the Filter Manager's name cache for the file name information.
	If the name is not found in the cache, FltGetFileNameInformation queries the file system and caches the result.

	FLT_FILE_NAME_QUERY_CACHE_ONLY
	FltGetFileNameInformation queries the Filter Manager's name cache for the file name information.
	FltGetFileNameInformation does not query the file system.

	FLT_FILE_NAME_QUERY_FILESYSTEM_ONLY
	FltGetFileNameInformation queries the file system for the file name information.
	FltGetFileNameInformation does not query the Filter Manager's name cache,
	and does not cache the result of the file system query.

	FLT_FILE_NAME_REQUEST_FROM_CURRENT_PROVIDER
	A name provider minifilter can use this flag to specify that a name query request should be redirected to itself
	(the name provider minifilter) rather than being satisfied by the name providers lower in the stack.

	FLT_FILE_NAME_ALLOW_QUERY_ON_REPARSE
	A name provider minifilter can use this flag to specify
	that it is safe to query the name in the post-create path even if STATUS_REPARSE was returned.
	It is the caller's responsibility to ensure that the FileObject->FileName field was not changed.
	Do not use this flag with mount points or symbolic link reparse points.
	This flag is available on Microsoft Windows Server 2003 SP1 and later.
	This flag is also available on Windows 2000 SP4 with Update Rollup 1 and later.
	*/
	if (KeGetCurrentIrql() > PASSIVE_LEVEL || IoGetTopLevelIrp() != NULL) {
		/*
		IoGetTopLevelIrp can return NULL, an arbitrary file-system-specific value
		(such as a pointer to the current IRP), or one of the flags listed in the following table.

		FSRTL_FSP_TOP_LEVEL_IRP			This is a recursive call.
		FSRTL_CACHE_TOP_LEVEL_IRP		The cache manager is the top-level component for the current thread.
		FSRTL_MOD_WRITE_TOP_LEVEL_IRP	The modified page writer is the top-level component for the current thread.
		FSRTL_FAST_IO_TOP_LEVEL_IRP		The cache manager is the top-level component for the current thread,
		and the current thread is in a fast I/O path.
		*/
		//__dlog("%-32s IRQL=%d", __func__, KeGetCurrentIrql());
		return NULL;
	}
	/*
	FILE_OBJECT Names in IRP_MJ_CREATE
	http://fsfilters.blogspot.com/2011/09/fileobject-names-in-irpmjcreate.html
	*/
	if (NULL == pCallbackData || NULL == pFltObjects || NULL == pFltObjects->FileObject ||
		NULL == pFltObjects->FileObject->FileName.Buffer)
	{
		//__dlog("%-32s NULL", __func__);
		return NULL;
	}
	if (pFltObjects->FileObject->FileName.Length <= 6 ||
		(
			pFltObjects->FileObject->FileName.Buffer[0] == L'\\'	&&
			pFltObjects->FileObject->FileName.Buffer[1] == L'$'		&&
			pFltObjects->FileObject->FileName.Buffer[2] != L'R'
			)
		)
	{
		//	NTFS Meta 정보 관련 오퍼레이션
		//__log("%-32s %wZ", __func__, &pFltObjects->FileObject->FileName);
		return NULL;
	}

	NTSTATUS	status = STATUS_UNSUCCESSFUL;
	BOOLEAN		bIsDirectory = false;
	FltIsDirectory(pFltObjects->FileObject, pFltObjects->Instance, &bIsDirectory);
	if (pbIsDirectory)	*pbIsDirectory = bIsDirectory ? true : false;

	PFLT_FILE_NAME_INFORMATION pFileNameInfo = NULL;
	__try
	{
		/*
		status = FltGetFileNameInformation(
			pCallbackData,
			FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT,
			&pFileNameInfo
		);
		*/
		if (NT_FAILED(status) || NULL == pFileNameInfo)
		{
			status = FltGetFileNameInformation(
				pCallbackData,
				FLT_FILE_NAME_OPENED | FLT_FILE_NAME_QUERY_ALWAYS_ALLOW_CACHE_LOOKUP,
				&pFileNameInfo
			);
			if (NT_FAILED(status) || NULL == pFileNameInfo)
			{
				//	STATUS_INVALID_PARAMETER 오류로 실패되는 경우가 많은데
				//	주로 어떤 경우일까요?
				__dlog("%s FltGetFileNameInformation() failed. status=%08x",
					__FUNCTION__, status);
				__leave;
			}
		}
		status = FltParseFileNameInformation(pFileNameInfo);
		if (NT_FAILED(status)) {
			__log("%s FltParseFileNameInformation() failed. status=%08x", __FUNCTION__, status);
			__leave;
		}
		if ((pFileNameInfo->Name.Buffer[0] == L'\\') &&
			(pFileNameInfo->Name.Buffer[1] == L'$') &&
			(pFileNameInfo->Name.Buffer[2] != L'R')
			)
		{
			//	\$Extend\$Quota:$Q:$INDEX_ALLOCATION
			//	이와 같은 파일명이 등장할 수 있다. 
			//	이건 NTFS metadata
			//	https://superuser.com/questions/1092378/what-is-c-extend-reparserindex-allocation-in-win7
			//	http://ntfs.com/ntfs-system-files.htm
			//	https://en.wikipedia.org/wiki/NTFS_reparse_point
			//	그런데 \$R 로 시작되는 것은 무엇인가?
			//__log("%s(2) %wZ", __FUNCTION__, &pFileNameInfo->Name);
			__leave;
		}
		status = STATUS_SUCCESS;
	}
	_finally
	{
		if (!NT_SUCCESS(status))
		{
			if (pFileNameInfo != NULL)
			{
				FltReleaseFileNameInformation(pFileNameInfo);
			}
			pFileNameInfo = NULL;
		}
	}
	return pFileNameInfo;
}

BOOLEAN	FLTAPI	IsWriteIONew
(
	ACCESS_MASK     accessMask,
	ULONG           disposition,
	ULONG			createOption
)
{
	BOOLEAN bWriteIo = TRUE;
	__try
	{
		if (disposition != FILE_OPEN)	// userlevel의 OPEN_EXISTING 에 해당.
			__leave;
		if ((accessMask & FILE_WRITE_DATA) || (accessMask & FILE_WRITE_ATTRIBUTES) || (accessMask & FILE_WRITE_EA) || (accessMask & FILE_APPEND_DATA))
			__leave;
		if ((accessMask & WRITE_OWNER) || (accessMask & WRITE_DAC))
			__leave;
		if (createOption & FILE_DELETE_ON_CLOSE)
			__leave;
		bWriteIo = FALSE;
	}
	__finally {
	
	
	}
	return bWriteIo;
}
BOOLEAN	FLTAPI	IsReadIO(
	ACCESS_MASK     accessMask,
	ULONG           disposition,
	ULONG			createOption
)
{
	UNREFERENCED_PARAMETER(createOption);
	BOOLEAN bReadIo = FALSE;
	__try
	{
		if (disposition != FILE_OPEN)	// userlevel의 OPEN_EXISTING 에 해당.
			__leave;
		if (accessMask & FILE_READ_DATA)
			bReadIo = TRUE;
	}
	__finally {

	}
	return bReadIo;
}


FLT_PREOP_CALLBACK_STATUS	PreCreate(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID* CompletionContext
)
{
	UNREFERENCED_PARAMETER(Data);
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);
	// 필터링 중지거나 커널 쓰레드인 경우 체크하지 않음 
	//	(추후 커널레벨에서의 방어가 필요한 경우 IoIsSystemThread 처리 부분 제거)

	if( IsNegligibleIO(Data->Thread)		||
		KernelMode == Data->RequestorMode	||
		FlagOn(Data->Iopb->OperationFlags, SL_OPEN_PAGING_FILE)
	)	
		return FLT_PREOP_SUCCESS_NO_CALLBACK;

	CFltObjectReference		filter(Config()->pFilter);
	if( filter.Reference() ) {
		ACCESS_MASK accessMask = Data->Iopb->Parameters.Create.SecurityContext->DesiredAccess;
		ULONG		disposition = (Data->Iopb->Parameters.Create.Options >> 24) & 0xFF;
		ULONG		createOption = Data->Iopb->Parameters.Create.Options & 0x00FFFFFF;

		PFLT_FILE_NAME_INFORMATION	pFileNameInfo = NULL;
		__try {
			if (false == IsWriteIO(accessMask, disposition, createOption))		__leave;		
			PY_FILE_CONTEXT		p	= CreateFileContext();

			*((PY_FILE_CONTEXT *)CompletionContext)	= p;
			//pFileNameInfo = FltGetFileName(Data, FltObjects, &bIsDirectory);
			//if( NULL == pFileNameInfo || NULL == pFileNameInfo->Name.Buffer )	__leave;
			//__log("%-32s %wZ", __func__, pFileNameInfo->Name);
		}
		__finally {
			if (pFileNameInfo)	FltReleaseFileNameInformation(pFileNameInfo);
		}	
	}
	return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}

FLT_POSTOP_CALLBACK_STATUS	PostCreateSafe(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_opt_ PVOID CompletionContext,
	_In_ FLT_POST_OPERATION_FLAGS Flags
)
{
	UNREFERENCED_PARAMETER(Data);
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);
	UNREFERENCED_PARAMETER(Flags);

	PFILE_OBJECT			pFileObject		= NULL;
	PDEVICE_OBJECT			pDeviceObject	= NULL;

	FLT_POSTOP_CALLBACK_STATUS	status	= FLT_POSTOP_FINISHED_PROCESSING;
	PFLT_FILE_NAME_INFORMATION	pFileNameInfo = NULL;

	PY_FILE_CONTEXT				pContext	= (PY_FILE_CONTEXT)CompletionContext;
	CFltObjectReference			filter(Config()->pFilter);

	__try {
		pFileObject		= Data->Iopb->TargetFileObject;
		if( pFileObject ) {
			pDeviceObject	= IoGetRelatedDeviceObject(pFileObject);
			if( pDeviceObject ) {
				if( FILE_DEVICE_MAILSLOT == pDeviceObject->DeviceType	||
					FILE_DEVICE_NAMED_PIPE == pDeviceObject->DeviceType )
					__leave;
			}	
		}
		if (!NT_SUCCESS( Data->IoStatus.Status ) || (STATUS_REPARSE == Data->IoStatus.Status)) 
			__leave;

		bool						bIsDirectory;		
		if( filter.Reference() ) {
			ACCESS_MASK accessMask = Data->Iopb->Parameters.Create.SecurityContext->DesiredAccess;
			ULONG		disposition = (Data->Iopb->Parameters.Create.Options >> 24) & 0xFF;
			ULONG		createOption = Data->Iopb->Parameters.Create.Options & 0x00FFFFFF;

			UNREFERENCED_PARAMETER(accessMask);
			UNREFERENCED_PARAMETER(disposition);
			UNREFERENCED_PARAMETER(createOption);

			pFileNameInfo = FltGetFileName(Data, FltObjects, &bIsDirectory);
			if( NULL == pFileNameInfo || NULL == pFileNameInfo->Name.Buffer )	{
				//__dlog("%-32s FltGetFileName() failed.", __func__);
				__leave;
			}
			/*
			BOOLEAN RtlTimeToSecondsSince1970(
  PLARGE_INTEGER Time,
  PULONG         ElapsedSeconds
);
			
			*/
			if( pContext ) {
				KeQuerySystemTime(&pContext->times[1]);
				if( false == bIsDirectory )	{
					//RtlTimeToSecondsSince1970()
					DWORD	s	= (DWORD)(pContext->times[1].QuadPart-pContext->times[0].QuadPart);
					if( s > 1000 ) {
						__log("%-32s %wZ", __func__, pFileNameInfo->Name);
						__log("  time       :%07d", s);
						__log("  access     :%08x", accessMask);
						__log("  disposition:%08x", disposition);
						__log("  create     :%08x", createOption);
					}
				}
			}
		}
	} __finally {
	
		if (pFileNameInfo)			FltReleaseFileNameInformation(pFileNameInfo);
		if( CompletionContext )		{
		
			CMemory::Free(CompletionContext);
		}
	}
	return status;
}
FLT_POSTOP_CALLBACK_STATUS	PostCreate(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_opt_ PVOID CompletionContext,
	_In_ FLT_POST_OPERATION_FLAGS Flags
)
{
	UNREFERENCED_PARAMETER(Data);
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);
	UNREFERENCED_PARAMETER(Flags);

	FLT_POSTOP_CALLBACK_STATUS	status	= FLT_POSTOP_FINISHED_PROCESSING;
	__try {
		if( FlagOn(Flags, FLTFL_POST_OPERATION_DRAINING)	||
			IsNegligibleIO(Data->Thread) )
			__leave;

		if( FALSE == FltDoCompletionProcessingWhenSafe(Data, FltObjects, CompletionContext, Flags, 
			PostCreateSafe, &status) ) {
			Data->IoStatus.Status = STATUS_UNSUCCESSFUL;
			Data->IoStatus.Information = 0;
		}
	} __finally {

	}
	return status;
}
FLT_PREOP_CALLBACK_STATUS	PreCleanup(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID* CompletionContext
)
{
	UNREFERENCED_PARAMETER(Data);
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);
	// 필터링 중지거나 커널 쓰레드인 경우 체크하지 않음 
	//	(추후 커널레벨에서의 방어가 필요한 경우 IoIsSystemThread 처리 부분 제거)

	if( IsNegligibleIO(Data->Thread)		||
		KernelMode == Data->RequestorMode	||
		FlagOn(Data->Iopb->OperationFlags, SL_OPEN_PAGING_FILE)
		)	
		return FLT_PREOP_SUCCESS_NO_CALLBACK;

	CFltObjectReference		filter(Config()->pFilter);
	if( filter.Reference() ) {

		__try {

		}
		__finally {

		}	
	}
	return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}
FLT_POSTOP_CALLBACK_STATUS	PostCleanupSafe (
	__inout PFLT_CALLBACK_DATA Data,
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__in_opt PVOID CompletionContext,
	__in FLT_POST_OPERATION_FLAGS Flags
)
{
	UNREFERENCED_PARAMETER(Data);
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);
	UNREFERENCED_PARAMETER(Flags);

	FLT_POSTOP_CALLBACK_STATUS	status	= FLT_POSTOP_FINISHED_PROCESSING;
	HANDLE						PID		= (HANDLE)FltGetRequestorProcess(Data);

	UNREFERENCED_PARAMETER(PID);

	__try {


	} __finally {

	}
	return status;

}

FLT_POSTOP_CALLBACK_STATUS	PostCleanup(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_opt_ PVOID CompletionContext,
	_In_ FLT_POST_OPERATION_FLAGS Flags
)
{
	UNREFERENCED_PARAMETER(Data);
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);
	UNREFERENCED_PARAMETER(Flags);

	FLT_POSTOP_CALLBACK_STATUS	status	= FLT_POSTOP_FINISHED_PROCESSING;
	__try {
		if( FlagOn(Flags, FLTFL_POST_OPERATION_DRAINING) )	__leave;

		if( FALSE == FltDoCompletionProcessingWhenSafe(Data, FltObjects, CompletionContext, Flags, 
			PostCleanupSafe, &status) ) {
			Data->IoStatus.Status = STATUS_UNSUCCESSFUL;
			Data->IoStatus.Information = 0;
		}
	} __finally {
	
	}
	return status;
}