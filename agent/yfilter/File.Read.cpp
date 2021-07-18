#include "yfilter.h"
#include "File.h"

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
		if( 0 == pStreamHandleContext->read.nCount ) {
			KeQuerySystemTime(&pStreamHandleContext->read.firstTime);		
		}
		InterlockedIncrement(&pStreamHandleContext->read.nCount);
		InterlockedAdd64(&pStreamHandleContext->read.nBytes, nBytes);
		InterlockedAdd64(&pStreamHandleContext->nBytes, nBytes);
		InterlockedIncrement(&pStreamHandleContext->nCount);
		KeQuerySystemTime(&pStreamHandleContext->read.lastTime);	

		//	STATUS_FLT_DISALLOW_FAST_IO
		if( NT_SUCCESS(Data->IoStatus.Status) ) {
			if( pStreamHandleContext->read.nLastOffset < offset.QuadPart + nBytes)
				InterlockedExchange64(&pStreamHandleContext->read.nLastOffset, offset.QuadPart+nBytes);
			if( pStreamHandleContext->read.nStartOffset > offset.QuadPart )
				InterlockedExchange64(&pStreamHandleContext->read.nStartOffset, offset.QuadPart);
		}
		else if( STATUS_END_OF_FILE == Data->IoStatus.Status ) {
			//	무시
			//__log("%-32s STATUS_END_OF_FILE %d", __func__, nBytes);		
		}
		else if( STATUS_FLT_DISALLOW_FAST_IO == Data->IoStatus.Status ) {
			//	이런 경우는 무시하시오
			// 
			//__log("STATUS_FLT_DISALLOW_FAST_IO %d", nBytes);		
		}
		else {
			__log("%-32s status %08x", __func__, Data->IoStatus.Status);
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