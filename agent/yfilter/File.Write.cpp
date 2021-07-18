#include "yfilter.h"
#include "File.h"

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
			InterlockedAdd64(&p->nBytes, Data->Iopb->Parameters.Write.Length);
			InterlockedIncrement(&p->nCount);

			if( p->bCreate ) {
				//__log("%-32s %d", __func__, Data->Iopb->Parameters.Write.Length);
			}
		}
		else if( STATUS_FLT_DISALLOW_FAST_IO == Data->IoStatus.Status ) {
			//	정상적인 IO로 IRP가 다시 올거라오
		}
		else {
			__log("%-32s Status:%08x", __func__, Data->IoStatus.Status);
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