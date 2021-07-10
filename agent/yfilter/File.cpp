#include "yfilter.h"


typedef struct Y_FILE_CONTEXT {
	LARGE_INTEGER		times[2];
	HANDLE				PID;
	HANDLE				TID;
	ULONG				SID;
} Y_FILE_CONTEXT, *PY_FILE_CONTEXT;

static	CFileTable		g_file;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(NONPAGE, FileTable)
#endif

CFileTable *	FileTable()
{
	return &g_file;
}
void			CreateFileTable()
{
	g_file.Initialize(false);
}
void			DestroyFileTable()
{
	g_file.Destroy();
}

volatile	LONG	g_nStreamHandle;
volatile	LONG	g_nFileContext;


PY_FILE_CONTEXT			CreateFileContext() {
	PY_FILE_CONTEXT		p	= (PY_FILE_CONTEXT)CMemory::Allocate(PagedPool, sizeof(Y_FILE_CONTEXT), 'elif');
	if( p ) {
		KeQuerySystemTime(&p->times[0]);
		//	ExSystemTimeToLocalTime
		p->times[1].QuadPart	= 0;
		p->PID					= PsGetCurrentProcessId();
		p->TID					= PsGetCurrentThreadId();	

		//__dlog("%-32s %06d	%p", __func__, InterlockedIncrement(&g_nFileContext), p);
	}
	return p;
}
void					ReleaseFileContext(PY_FILE_CONTEXT p) {
	if( p )	{
		CMemory::Free(p);
		//__dlog("%-32s %06d	%p", __func__, InterlockedDecrement(&g_nFileContext), p);
	}
}

PY_STREAMHANDLE_CONTEXT	CreateStreamHandle() {
	if( NULL == Config() )	return NULL;
	PY_STREAMHANDLE_CONTEXT		p	= NULL;

	NTSTATUS		status;
	status	= FltAllocateContext(Config()->pFilter, FLT_STREAMHANDLE_CONTEXT, 
				sizeof(Y_STREAMHANDLE_CONTEXT), PagedPool, (PFLT_CONTEXT *)&p);
	if( NT_SUCCESS(status))	{
		RtlZeroMemory(p, sizeof(Y_STREAMHANDLE_CONTEXT));

		//__log("%-32s %06d	%p", __func__, InterlockedIncrement(&g_nStreamHandle), p);
	}
	else {
		__dlog("%-32s FltAllocateContext() failed. result=%08x", __func__, status);
	}
	return p;
}
void					DeleteStreamHandleCallback(PY_STREAMHANDLE_CONTEXT p) {

	CWSTRBuffer	buf;
	if (p->pFileNameInfo) {
		RtlStringCbCopyUnicodeString((PWSTR)buf, buf.CbSize(), &p->pFileNameInfo->Name);
		FltReleaseFileNameInformation(p->pFileNameInfo);	
		p->pFileNameInfo	= NULL;
	}	
	if( ProcessTable()->IsExisting(p->PID, p, [](
		bool					bCreationSaved,
		IN PPROCESS_ENTRY		pEntry,			
		IN PVOID				pContext		
		) {
		UNREFERENCED_PARAMETER(bCreationSaved);
		UNREFERENCED_PARAMETER(pEntry);
		PY_STREAMHANDLE_CONTEXT		p	= (PY_STREAMHANDLE_CONTEXT)pContext;	
		
		UNREFERENCED_PARAMETER(p);
	}) ) {
	
	}
	else
	{
		__log("%-32s PID:%d is not found.", __func__, (DWORD)p->PID);		
	}

	if( false == p->bIsDirectory ) {
		if( false ) {
			__log("%05d %ws", p->PID, (PWSTR)buf);
			if( p->read.nCount ) {
				__log("  read");
				__log("  nCount:%d", p->read.nCount);
				__log("  nBytes:%I64d", p->read.nBytes);
			}
			if( p->write.nCount ) {
				__log("  write");
				__log("  nCount:%d", p->write.nCount);
				__log("  nBytes:%I64d", p->write.nBytes);
			}
		}
	}
	//__log("%-32s %06d	%p", __func__, InterlockedDecrement(&g_nStreamHandle), p);
}
void		CreateFileMessage(
	PFILE_ENTRY			p,
	PY_FILE_MESSAGE		*pOut
)
{
	WORD	wDataSize	= sizeof(Y_FILE_DATA);
	WORD	wStringSize	= sizeof(Y_FILE_STRING);
	wStringSize		+=	GetStringDataSize(&p->FilePath);

	PY_FILE_MESSAGE		pMsg	= NULL;
	HANDLE				PPID	= GetParentProcessId(p->PID);

	pMsg = (PY_FILE_MESSAGE)CMemory::Allocate(PagedPool, wDataSize+wStringSize, TAG_PROCESS);
	if (pMsg) 
	{
		//__dlog("%-32s message size:%d", __func__, wPacketSize);
		//__dlog("  header :%d", sizeof(Y_HEADER));
		//__dlog(" struct  :%d", sizeof(Y_REGISTRY));
		//__dlog(" regpath :%d", GetStringDataSize(pRegPath));
		//__dlog(" value   :%d", GetStringDataSize(pRegValueName));

		RtlZeroMemory(pMsg, wDataSize+wStringSize);

		pMsg->mode		= YFilter::Message::Mode::Event;
		pMsg->category	= YFilter::Message::Category::Registry;
		//pMsg->subType	= FileTable()->Class2SubType(p->nClass);
		pMsg->wDataSize	= wDataSize;
		pMsg->wStringSize	= wStringSize;
		pMsg->wRevision	= 1;

		pMsg->PUID		= p->PUID;
		pMsg->PID		= (DWORD)p->PID;
		pMsg->CPID		= (DWORD)p->PID;
		pMsg->RPID		= (DWORD)0;
		pMsg->PPID		= (DWORD)PPID;
		pMsg->CTID		= (DWORD)p->TID;
		pMsg->TID		= pMsg->CTID;

		pMsg->FileUID	= p->FileUID;
		pMsg->FilePUID	= p->FilePUID;
		pMsg->nCount	= p->nCount;
		pMsg->nSize		= p->dwSize;
		WORD		dwStringOffset	= (WORD)(sizeof(Y_REGISTRY_MESSAGE));

		pMsg->Path.wOffset	= dwStringOffset;
		pMsg->Path.wSize	= GetStringDataSize(&p->FilePath);
		pMsg->Path.pBuf		= (WCHAR *)((char *)pMsg + dwStringOffset);
		RtlStringCbCopyUnicodeString(pMsg->Path.pBuf, pMsg->Path.wSize, &p->FilePath);
		if( pOut ) {
			*pOut	= pMsg;
		}
		else {
			CMemory::Free(pMsg);
		}
	}
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

FLT_PREOP_CALLBACK_STATUS	PreRead(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID* CompletionContext
)
{
	UNREFERENCED_PARAMETER(Data);
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);
	
	//return FLT_PREOP_SUCCESS_NO_CALLBACK;
	return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}
FLT_POSTOP_CALLBACK_STATUS	PostReadSafe(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_opt_ PVOID CompletionContext,
	_In_ FLT_POST_OPERATION_FLAGS Flags
)
{
	UNREFERENCED_PARAMETER(Flags);
	UNREFERENCED_PARAMETER(CompletionContext);
	FLT_POSTOP_CALLBACK_STATUS	status	= FLT_POSTOP_FINISHED_PROCESSING;
	HANDLE						PID		= (HANDLE)FltGetRequestorProcess(Data);
	PY_STREAMHANDLE_CONTEXT		pStreamHandleContext	= NULL;

	UNREFERENCED_PARAMETER(PID);

	LARGE_INTEGER				offset	= {0,};
	ULONG						nBytes	= 0;

	__try {
		//	[TODO]	delete stream handle
		if( NT_SUCCESS(FltGetStreamHandleContext(FltObjects->Instance, Data->Iopb->TargetFileObject, 
			(PFLT_CONTEXT *)&pStreamHandleContext)) ) {

		}
		else {
			__leave;
		}

		switch( Data->Iopb->MajorFunction ) {
			case IRP_MJ_MDL_READ:
				offset.QuadPart		= Data->Iopb->Parameters.MdlRead.FileOffset.QuadPart;
				nBytes				= Data->Iopb->Parameters.MdlRead.Length;
			break;

			case IRP_MJ_READ:
				offset.QuadPart		= Data->Iopb->Parameters.Read.ByteOffset.QuadPart;
				nBytes				= (ULONG)Data->IoStatus.Information;
			break;
		
		}
		if( pStreamHandleContext->read.nCount ) {
			KeQuerySystemTime(&pStreamHandleContext->read.firstTime);		
		}
		InterlockedIncrement(&pStreamHandleContext->read.nCount);
		InterlockedAdd64(&pStreamHandleContext->read.nBytes, nBytes);
		KeQuerySystemTime(&pStreamHandleContext->read.lastTime);	
		if( NT_SUCCESS(Data->IoStatus.Status) ) {
			if( pStreamHandleContext->read.nLastOffset < offset.QuadPart + nBytes)
				InterlockedExchange64(&pStreamHandleContext->read.nLastOffset, offset.QuadPart+nBytes);
			if( pStreamHandleContext->read.nStartOffset > offset.QuadPart )
				InterlockedExchange64(&pStreamHandleContext->read.nStartOffset, offset.QuadPart);
		}
	}
	__finally {
		if( pStreamHandleContext )		FltReleaseContext(pStreamHandleContext);

	}
	return status;
}
FLT_POSTOP_CALLBACK_STATUS	PostRead(
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
	__try
	{
		if( FLT_IS_FASTIO_OPERATION(Data) ) {
			//__dlog("%-32s FLT_IS_FASTIO_OPERATION", __func__);
			status	= PostReadSafe(Data, FltObjects, CompletionContext, Flags);		
			__leave;
		}
		if( FALSE == FltDoCompletionProcessingWhenSafe(Data, FltObjects, CompletionContext, Flags, 
			PostReadSafe, &status) ) {
			Data->IoStatus.Status = STATUS_UNSUCCESSFUL;
			Data->IoStatus.Information = 0;
		}
	}
	__finally {
	
	
	}
	return status;
}
FLT_PREOP_CALLBACK_STATUS	PreWrite(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID* CompletionContext
)
{
	UNREFERENCED_PARAMETER(Data);
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);

	//return FLT_PREOP_SUCCESS_NO_CALLBACK;
	return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}
FLT_POSTOP_CALLBACK_STATUS	PostWriteSafe(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_opt_ PVOID CompletionContext,
	_In_ FLT_POST_OPERATION_FLAGS Flags
)
{
	UNREFERENCED_PARAMETER(Flags);
	UNREFERENCED_PARAMETER(CompletionContext);
	FLT_POSTOP_CALLBACK_STATUS	status	= FLT_POSTOP_FINISHED_PROCESSING;
	HANDLE						PID		= (HANDLE)FltGetRequestorProcess(Data);
	PY_STREAMHANDLE_CONTEXT		p	= NULL;

	UNREFERENCED_PARAMETER(PID);

	__try {
		if( NT_SUCCESS(FltGetStreamHandleContext(FltObjects->Instance, Data->Iopb->TargetFileObject, 
			(PFLT_CONTEXT *)&p)) ) {

		}
		else {
			//__dlog("%-32s pStreamHandleContext:NULL", __func__);
			__leave;
		}

		if( NT_SUCCESS(Data->IoStatus.Status) ) {
			InterlockedIncrement(&p->write.nCount);
			InterlockedAdd64(&p->write.nBytes, Data->Iopb->Parameters.Write.Length);
		}
		else {
			//__log("%-32s FAILED", __func__);
		}
	}
	__finally {
		if( p )		FltReleaseContext(p);

	}
	return status;
}
FLT_POSTOP_CALLBACK_STATUS	PostWrite(
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

	__try
	{
		if( FALSE == FltDoCompletionProcessingWhenSafe(Data, FltObjects, CompletionContext, Flags, 
			PostWriteSafe, &status) ) {
			Data->IoStatus.Status = STATUS_UNSUCCESSFUL;
			Data->IoStatus.Information = 0;
		}
	}
	__finally {


	}
	return status;
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

			if( p ) {
				*((PY_FILE_CONTEXT *)CompletionContext)	= p;
				p->PID	= (HANDLE)FltGetRequestorProcessId(Data);
				FltGetRequestorSessionId(Data, &p->SID);
				p->TID	= PsGetThreadId(Data->Thread);
			}
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

	FLT_POSTOP_CALLBACK_STATUS	ret	= FLT_POSTOP_FINISHED_PROCESSING;
	PY_FILE_CONTEXT				pContext		= (PY_FILE_CONTEXT)CompletionContext;
	PY_STREAMHANDLE_CONTEXT		p	= NULL;
	NTSTATUS					status	= STATUS_SUCCESS;
	PFLT_FILE_NAME_INFORMATION	pFileNameInfo	= NULL;

	FILE_CALLBACK_ARG			arg	= {NULL,};

	arg.PID		= (HANDLE)FltGetRequestorProcessId(Data);
	FltGetRequestorSessionId(Data, &arg.SID);
	arg.TID		= PsGetThreadId(Data->Thread);

	__try {
		if (!NT_SUCCESS( Data->IoStatus.Status ) || (STATUS_REPARSE == Data->IoStatus.Status)) 
			__leave;

		status = FltGetFileNameInformation(
					Data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_ALWAYS_ALLOW_CACHE_LOOKUP,
					&pFileNameInfo);
		if( NT_FAILED(status))	__leave;
		status	= FltParseFileNameInformation(pFileNameInfo);
		if( NT_FAILED(status))	__leave;

		p	= CreateStreamHandle();
		if( NULL == p )	__leave;

		status = FltSetStreamHandleContext(FltObjects->Instance, Data->Iopb->TargetFileObject,
					FLT_SET_CONTEXT_REPLACE_IF_EXISTS, p, NULL);
		if( NT_FAILED(status)) {
			FltReleaseContext(p);	
			p	= NULL;
			__leave;
		}	
		arg.pFilePath	= &pFileNameInfo->Name;
		if( !ProcessTable()->IsExisting(arg.PID, &arg, [](
			bool					bCreationSaved,
			IN PPROCESS_ENTRY		pEntry,			
			IN PVOID				pContext		
			) {
			UNREFERENCED_PARAMETER(bCreationSaved);
			PFILE_CALLBACK_ARG		p	= (PFILE_CALLBACK_ARG)pContext;			
			p->PUID		= pEntry->PUID;
		}) ) {
			__log("%-32s PID:%d is not found.", __func__, (DWORD)arg.PID);		
		}

		p->PID				= arg.PID;
		p->PUID				= arg.PUID;
		p->TID				= arg.TID;
		p->pFileNameInfo	= pFileNameInfo;
		FltReferenceFileNameInformation(p->pFileNameInfo);

		FltIsDirectory(Data->Iopb->TargetFileObject, FltObjects->Instance, &p->bIsDirectory);
		p->accessMask	= Data->Iopb->Parameters.Create.SecurityContext->DesiredAccess;
		p->disposition	= (Data->Iopb->Parameters.Create.Options >> 24) & 0xFF;
		p->createOption	= Data->Iopb->Parameters.Create.Options & 0x00FFFFFF;

			//p->pFileNameInfo	= pFileNameInfo;
			//pFileNameInfo		= NULL;
			/*
			BOOLEAN RtlTimeToSecondsSince1970(
  PLARGE_INTEGER Time,
  PULONG         ElapsedSeconds
);
			
			*/
		if( pContext ) {
			KeQuerySystemTime(&pContext->times[1]);
			if( false == p->bIsDirectory )	{
				//RtlTimeToSecondsSince1970()
				DWORD	s	= (DWORD)(pContext->times[1].QuadPart-pContext->times[0].QuadPart);
				if( false || s > 10000 ) {
					__log("%-32s %wZ", __func__, p->pFileNameInfo->Name);
					__log("  time       :%07d", s);
					__log("  access     :%08x", p->accessMask);
					__log("  disposition:%08x", p->disposition);
					__log("  create     :%08x", p->createOption);
				}
			}
		}

		ULONG	nCount	= 0;
		if( FileTable()->Add(&arg, &nCount) ) {
			//__log("%-32s %d", "File::PostCreateSafe", nCount);

		}
		else {
			//__log("%-32s FileTable()->Add() failed.", __func__);		
		}
	} __finally {
	
		if (pFileNameInfo)	FltReleaseFileNameInformation(pFileNameInfo);
		if( p)				FltReleaseContext(p);
	}
	return ret;
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
	PFILE_OBJECT			pFileObject		= NULL;
	PDEVICE_OBJECT			pDeviceObject	= NULL;

	__try {
		if( FlagOn(Flags, FLTFL_POST_OPERATION_DRAINING)	||
			IsNegligibleIO(Data->Thread) )
			__leave;

		pFileObject		= Data->Iopb->TargetFileObject;
		if( pFileObject ) {
			pDeviceObject	= IoGetRelatedDeviceObject(pFileObject);
			if( pDeviceObject ) {
				if( FILE_DEVICE_MAILSLOT == pDeviceObject->DeviceType	||
					FILE_DEVICE_NAMED_PIPE == pDeviceObject->DeviceType ) {
					//	[TODO]
					//	IRP_MJ_CREATE_MAILSLOT이 따로 있는데요?

					__dlog("%-32s DeviceType:%d", __func__, pDeviceObject->DeviceType);
					__leave;
				}
			}	
		}
		if( FALSE == FltDoCompletionProcessingWhenSafe(Data, FltObjects, CompletionContext, Flags, 
			PostCreateSafe, &status) ) {
			Data->IoStatus.Status = STATUS_UNSUCCESSFUL;
			Data->IoStatus.Information = 0;
		}
	} __finally {
		if( CompletionContext )
			ReleaseFileContext((PY_FILE_CONTEXT)CompletionContext);

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

	//__log("%-32s", __func__);

	if( IsNegligibleIO(Data->Thread)		||
		KernelMode == Data->RequestorMode	||
		FlagOn(Data->Iopb->OperationFlags, SL_OPEN_PAGING_FILE)
		) {
		__log("%-32s skip", __func__);

		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}
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

	FLT_POSTOP_CALLBACK_STATUS	status		= FLT_POSTOP_FINISHED_PROCESSING;
	HANDLE						PID			= (HANDLE)FltGetRequestorProcess(Data);
	PY_STREAMHANDLE_CONTEXT		pContext	= NULL;

	UNREFERENCED_PARAMETER(PID);
	__try {
		if (FlagOn(Flags, FLTFL_POST_OPERATION_DRAINING))
		{
			__leave;
		}
		//	[TODO]	delete stream handle
		if( NT_SUCCESS(FltDeleteStreamHandleContext(FltObjects->Instance, Data->Iopb->TargetFileObject, 
			(PFLT_CONTEXT *)&pContext)) ) {
			//__log("%-32s %wZ", __func__, &pContext->pFileNameInfo->Name);
		}
		else {
			//__log("%-32s FltDeleteStreamHandleContext() failed.", __func__);
			__leave;
		}

		if (!NT_SUCCESS( Data->IoStatus.Status ) || (STATUS_REPARSE == Data->IoStatus.Status)) {
			__leave;
		}

	} __finally {
		if( pContext ) {			
			FileTable()->Remove(pContext->PUID, true, pContext, 
				[](PFILE_ENTRY pEntry,PVOID	p) {
				UNREFERENCED_PARAMETER(pEntry);
				PY_STREAMHANDLE_CONTEXT	pContext	= (PY_STREAMHANDLE_CONTEXT)p;
				if( pContext ) {
					
				
				}
				if( pEntry )
					__dlog("%-32s %p removed", __func__, pEntry->PUID);
			
			});
			FltReleaseContext(pContext);
		}
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