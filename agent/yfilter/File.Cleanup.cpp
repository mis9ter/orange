#include "yfilter.h"
#include "File.h"

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

	if( CFile::IsNegligibleIO(Data->Thread)		||
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
			if( KernelMode == pContext->RequestorMode ) {
				__log("%-32s %06d %10s C:%d D:%d %wZ", 
					__func__, pContext->PID, 
					(KernelMode == pContext->RequestorMode)? "Kernel":"User",
					pContext->bCreate, pContext->bIsDirectory, 
					&pContext->pFileNameInfo->Name);
			}
			//if( KernelMode == Data->RequestorMode)
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
			if( FileTable()->Remove(Data->Iopb->TargetFileObject, true, pContext, 
				[](PFILE_ENTRY pEntry,PVOID	p) {
				UNREFERENCED_PARAMETER(pEntry);
				PY_STREAMHANDLE_CONTEXT	pContext	= (PY_STREAMHANDLE_CONTEXT)p;
				if( pContext ) {
					
				
				}			
			}) ) {
				//if( !pContext->bIsDirectory && pContext->nBytes ) 
				if( false )
				{
					__log("%-32s %wZ", __func__, &pContext->pFileNameInfo->Name);
					__log("  PID         %d", pContext->PID);
					__log("  Object      %p", Data->Iopb->TargetFileObject);
					__log("  create      %d", pContext->bCreate);
					__log("  read.count  %d", pContext->read.nCount);
					__log("  read.size   %d", (int)(pContext->read.nBytes));
					__log("  write.count %d", pContext->write.nCount);
					__log("  write.size  %d", (int)(pContext->write.nBytes));
				}	
				if( pContext->bCreate ) {
					__log("%-32s %wZ", __func__, &pContext->pFileNameInfo->Name);
				}
				if( Config()->client.event.nConnected ) {
					PY_FILE_MESSAGE	pMsg	= NULL;
					CreateFileMessage(pContext, &pMsg);
					if( pMsg ) {
						if( MessageThreadPool()->Push(__func__, pMsg->mode, pMsg->category, 
							pMsg, pMsg->wDataSize+pMsg->wStringSize, false) ) {
							MessageThreadPool()->Alert(pMsg->category);
							pMsg	= NULL;
						}
						else {
							CMemory::Free(pMsg);
						}
					}	
				}
			}
			else {
				__log("%-32s %p can not remove.", __func__, Data->Iopb->TargetFileObject);
			
			}
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