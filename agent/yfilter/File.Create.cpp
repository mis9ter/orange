#include "yfilter.h"
#include "File.h"

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

	if( CFile::IsNegligibleIO(Data->Thread)		||
		KernelMode == Data->RequestorMode	||
		FlagOn(Data->Iopb->OperationFlags, SL_OPEN_PAGING_FILE)
	)	
		return FLT_PREOP_SUCCESS_NO_CALLBACK;

	//CFltObjectReference		filter(Config()->pFilter);
	//if( filter.Reference() ) 
	{
		ACCESS_MASK accessMask = Data->Iopb->Parameters.Create.SecurityContext->DesiredAccess;
		ULONG		disposition = (Data->Iopb->Parameters.Create.Options >> 24) & 0xFF;
		ULONG		createOption = Data->Iopb->Parameters.Create.Options & 0x00FFFFFF;

		PFLT_FILE_NAME_INFORMATION	pFileNameInfo = NULL;
		__try {
			if (false == CFile::IsWriteIO(accessMask, disposition, createOption))		__leave;	
			/*
			* 딱히 PreCreate 단계에서 생성할 거리가 없어서 일단 보류
			PY_FILE_CONTEXT		p	= CreateFileContext();
			if( p ) {
				*((PY_FILE_CONTEXT *)CompletionContext)	= p;
				p->PID	= (HANDLE)FltGetRequestorProcessId(Data);
				FltGetRequestorSessionId(Data, &p->SID);

				//	https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/fltkernel/ns-fltkernel-_flt_callback_data
				//	Data->Thread는 NULL일 수 있다. 
				if( Data->Thread ) 
					p->TID	= PsGetThreadId(Data->Thread);
				else {
					__log("%-32s Data->Thread == NULL", __func__);
					p->TID	= NULL;
				}
			}
			*/
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
	PY_STREAMHANDLE_CONTEXT		pStreamHandle	= NULL;
	NTSTATUS					status			= STATUS_SUCCESS;
	PFLT_FILE_NAME_INFORMATION	pFileNameInfo	= NULL;
	FILE_CALLBACK_ARG			arg				= {NULL,};

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

		pStreamHandle	= CreateStreamHandle();
		if( NULL == pStreamHandle )	__leave;

		status = FltSetStreamHandleContext(FltObjects->Instance, Data->Iopb->TargetFileObject,
					FLT_SET_CONTEXT_REPLACE_IF_EXISTS, pStreamHandle, NULL);
		if( NT_FAILED(status)) {
			FltReleaseContext(pStreamHandle);	
			pStreamHandle	= NULL;
			__leave;
		}	
		arg.PID	= (HANDLE)FltGetRequestorProcessId(Data);
		FltGetRequestorSessionId(Data, &arg.SID);
		//	https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/fltkernel/ns-fltkernel-_flt_callback_data
		//	Data->Thread는 NULL일 수 있다. 
		if( Data->Thread ) 
			arg.TID	= PsGetThreadId(Data->Thread);
		else {
			__log("%-32s Data->Thread == NULL", __func__);
			arg.TID	= NULL;
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
		pStreamHandle->PID				= arg.PID;
		pStreamHandle->PUID				= arg.PUID;
		pStreamHandle->TID				= arg.TID;
		pStreamHandle->RequestorMode	= Data->RequestorMode;
		pStreamHandle->pFileNameInfo	= pFileNameInfo;
		FltReferenceFileNameInformation(pStreamHandle->pFileNameInfo);
		FltIsDirectory(Data->Iopb->TargetFileObject, FltObjects->Instance, &pStreamHandle->bIsDirectory);
		pStreamHandle->accessMask	= Data->Iopb->Parameters.Create.SecurityContext->DesiredAccess;
		pStreamHandle->disposition	= (Data->Iopb->Parameters.Create.Options >> 24) & 0xFF;
		pStreamHandle->createOption	= Data->Iopb->Parameters.Create.Options & 0x00FFFFFF;

		if( FILE_CREATED == Data->IoStatus.Information ) {
			pStreamHandle->bCreate	= true;
		} else {
			
		}
		if( pStreamHandle->bCreate ) {
			__log("%-32s create", __func__);
			__log("  %wZ", &pStreamHandle->pFileNameInfo->Name);
			__log("  PID:%d", pStreamHandle->PID);
			__log("  %s", Data->RequestorMode==KernelMode? "KernelMode":"UserMode");
		}
		//p->pFileNameInfo	= pFileNameInfo;
			//pFileNameInfo		= NULL;
			/*
			BOOLEAN RtlTimeToSecondsSince1970(
  PLARGE_INTEGER Time,
  PULONG         ElapsedSeconds
);
			
			*/
		if( pContext ) {
			//	Pre / Post 간 시간 측정을 위해서 
			KeQuerySystemTime(&pContext->times[1]);
			if( false == pStreamHandle->bIsDirectory )	{
				//RtlTimeToSecondsSince1970()
				DWORD	s	= (DWORD)(pContext->times[1].QuadPart-pContext->times[0].QuadPart);
				if( false || s > 10000 ) {
					__log("%-32s %wZ", __func__, pStreamHandle->pFileNameInfo->Name);
					__log("  time       :%07d", s);
					__log("  access     :%08x", pStreamHandle->accessMask);
					__log("  disposition:%08x", pStreamHandle->disposition);
					__log("  create     :%08x", pStreamHandle->createOption);
				}
			}
		}
		ULONG	nCount	= 0;
		if( FileTable()->Add(Data->Iopb->TargetFileObject, &arg, &nCount) ) {
			//__log("%-32s %d", "File::PostCreateSafe", nCount);
		}
		else {
			__log("%-32s %p can not add.", __func__, Data->Iopb->TargetFileObject);		
		}
		pStreamHandle->FPUID	= arg.FPUID;
		pStreamHandle->FUID		= arg.FUID;
	} __finally {
	
		if (pFileNameInfo)	FltReleaseFileNameInformation(pFileNameInfo);
		if( pStreamHandle)	FltReleaseContext(pStreamHandle);
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
			CFile::IsNegligibleIO(Data->Thread) )
			__leave;
		if( !NT_SUCCESS(Data->IoStatus.Status)	||
			(STATUS_REPARSE == Data->IoStatus.Status) ) {
			__leave;
		}

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