#include "yfilter.h"

CONST FLT_OPERATION_REGISTRATION Callbacks[] = {

	{
		IRP_MJ_CREATE,
		FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO ,
		PreCreate,
		PostCreate 
	},
	{
		IRP_MJ_CLEANUP,
		FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO,
		PreCleanup,
		PostCleanup
	},
	{ 
		IRP_MJ_READ,
		FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO,
		PreRead,
		PostRead
	},
	{ 
		IRP_MJ_MDL_READ,
		FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO,
		PreRead,
		PostRead 
	},
	{ 
		IRP_MJ_MDL_READ_COMPLETE,
		0,
		NULL,
		NULL 
	},
	{ 
		IRP_MJ_WRITE,
		FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO,
		PreWrite,
		PostWrite
	},

#if 0 // TODO - List all of the requests to filter.
	{ IRP_MJ_CREATE_NAMED_PIPE,
	  0,
	  PreOperation,
	  PostOperation },

	{ IRP_MJ_CLOSE,
	  0,
	  PreOperation,
	  PostOperation },


	{ IRP_MJ_WRITE,
	  0,
	  PreOperation,
	  PostOperation },

	{ IRP_MJ_QUERY_INFORMATION,
	  0,
	  PreOperation,
	  PostOperation },

	{ IRP_MJ_SET_INFORMATION,
	  0,
	  PreOperation,
	  PostOperation },

	{ IRP_MJ_QUERY_EA,
	  0,
	  PreOperation,
	  PostOperation },

	{ IRP_MJ_SET_EA,
	  0,
	  PreOperation,
	  PostOperation },

	{ IRP_MJ_FLUSH_BUFFERS,
	  0,
	  PreOperation,
	  PostOperation },

	{ IRP_MJ_QUERY_VOLUME_INFORMATION,
	  0,
	  PreOperation,
	  PostOperation },

	{ IRP_MJ_SET_VOLUME_INFORMATION,
	  0,
	  PreOperation,
	  PostOperation },

	{ IRP_MJ_DIRECTORY_CONTROL,
	  0,
	  PreOperation,
	  PostOperation },

	{ IRP_MJ_FILE_SYSTEM_CONTROL,
	  0,
	  PreOperation,
	  PostOperation },

	{ IRP_MJ_DEVICE_CONTROL,
	  0,
	  PreOperation,
	  PostOperation },

	{ IRP_MJ_INTERNAL_DEVICE_CONTROL,
	  0,
	  PreOperation,
	  PostOperation },

	{ IRP_MJ_SHUTDOWN,
	  0,
	  PreOperationNoPostOperation,
	  NULL },                               //post operations not supported

	{ IRP_MJ_LOCK_CONTROL,
	  0,
	  PreOperation,
	  PostOperation },

	{ IRP_MJ_CREATE_MAILSLOT,
	  0,
	  PreOperation,
	  PostOperation },

	{ IRP_MJ_QUERY_SECURITY,
	  0,
	  PreOperation,
	  PostOperation },

	{ IRP_MJ_SET_SECURITY,
	  0,
	  PreOperation,
	  PostOperation },

	{ IRP_MJ_QUERY_QUOTA,
	  0,
	  PreOperation,
	  PostOperation },

	{ IRP_MJ_SET_QUOTA,
	  0,
	  PreOperation,
	  PostOperation },

	{ IRP_MJ_PNP,
	  0,
	  PreOperation,
	  PostOperation },

	{ IRP_MJ_ACQUIRE_FOR_SECTION_SYNCHRONIZATION,
	  0,
	  PreOperation,
	  PostOperation },

	{ IRP_MJ_RELEASE_FOR_SECTION_SYNCHRONIZATION,
	  0,
	  PreOperation,
	  PostOperation },

	{ IRP_MJ_ACQUIRE_FOR_MOD_WRITE,
	  0,
	  PreOperation,
	  PostOperation },

	{ IRP_MJ_RELEASE_FOR_MOD_WRITE,
	  0,
	  PreOperation,
	  PostOperation },

	{ IRP_MJ_ACQUIRE_FOR_CC_FLUSH,
	  0,
	  PreOperation,
	  PostOperation },

	{ IRP_MJ_RELEASE_FOR_CC_FLUSH,
	  0,
	  PreOperation,
	  PostOperation },

	{ IRP_MJ_FAST_IO_CHECK_IF_POSSIBLE,
	  0,
	  PreOperation,
	  PostOperation },

	{ IRP_MJ_NETWORK_QUERY_OPEN,
	  0,
	  PreOperation,
	  PostOperation },

	{ IRP_MJ_MDL_READ,
	  0,
	  PreOperation,
	  PostOperation },

	{ IRP_MJ_MDL_READ_COMPLETE,
	  0,
	  PreOperation,
	  PostOperation },

	{ IRP_MJ_PREPARE_MDL_WRITE,
	  0,
	  PreOperation,
	  PostOperation },

	{ IRP_MJ_MDL_WRITE_COMPLETE,
	  0,
	  PreOperation,
	  PostOperation },

	{ IRP_MJ_VOLUME_MOUNT,
	  0,
	  PreOperation,
	  PostOperation },

	{ IRP_MJ_VOLUME_DISMOUNT,
	  0,
	  PreOperation,
	  PostOperation },

#endif // TODO

	{ IRP_MJ_OPERATION_END }
};

//
//  This defines what we want to filter with FltMgr
//

CONST FLT_REGISTRATION FilterRegistration = {

	sizeof(FLT_REGISTRATION),		//  Size
	FLT_REGISTRATION_VERSION,		//  Version
	0,								//  Flags
	ContextRegistration,			//  Context
	Callbacks,						//  Operation callbacks
	DriverUnload,					//  MiniFilterUnload
	InstanceSetup,					//  InstanceSetup
	InstanceQueryTeardown,			//  InstanceQueryTeardown
	InstanceTeardownStart,			//  InstanceTeardownStart
	InstanceTeardownComplete,		//  InstanceTeardownComplete

	NULL,							//  GenerateFileName
	NULL,							//  GenerateDestinationFileName
	NULL							//  NormalizeNameComponent

};

#if 0

/*************************************************************************
	MiniFilter callback routines.
*************************************************************************/
FLT_PREOP_CALLBACK_STATUS
PreOperation(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID *CompletionContext
)
/*++

Routine Description:

	This routine is a pre-operation dispatch routine for this miniFilter.

	This is non-pageable because it could be called on the paging path

Arguments:

	Data - Pointer to the filter callbackData that is passed to us.

	FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
		opaque handles to this filter, instance, its associated volume and
		file object.

	CompletionContext - The context for the completion routine for this
		operation.

Return Value:

	The return value is the status of the operation.

--*/
{
	NTSTATUS status;

	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);

	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,
		("!PreOperation: Entered\n"));

	//
	//  See if this is an operation we would like the operation status
	//  for.  If so request it.
	//
	//  NOTE: most filters do NOT need to do this.  You only need to make
	//        this call if, for example, you need to know if the oplock was
	//        actually granted.
	//

	if (DoRequestOperationStatus(Data)) {

		status = FltRequestOperationStatusCallback(Data,
			OperationStatusCallback,
			(PVOID)(++OperationStatusCtx));
		if (!NT_SUCCESS(status)) {

			PT_DBG_PRINT(PTDBG_TRACE_OPERATION_STATUS,
				("!PreOperation: FltRequestOperationStatusCallback Failed, status=%08x\n",
					status));
		}
	}

	// This template code does not do anything with the callbackData, but
	// rather returns FLT_PREOP_SUCCESS_WITH_CALLBACK.
	// This passes the request down to the next miniFilter in the chain.

	return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}



VOID
OperationStatusCallback(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ PFLT_IO_PARAMETER_BLOCK ParameterSnapshot,
	_In_ NTSTATUS OperationStatus,
	_In_ PVOID RequesterContext
)
/*++

Routine Description:

	This routine is called when the given operation returns from the call
	to IoCallDriver.  This is useful for operations where STATUS_PENDING
	means the operation was successfully queued.  This is useful for OpLocks
	and directory change notification operations.

	This callback is called in the context of the originating thread and will
	never be called at DPC level.  The file object has been correctly
	referenced so that you can access it.  It will be automatically
	dereferenced upon return.

	This is non-pageable because it could be called on the paging path

Arguments:

	FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
		opaque handles to this filter, instance, its associated volume and
		file object.

	RequesterContext - The context for the completion routine for this
		operation.

	OperationStatus -

Return Value:

	The return value is the status of the operation.

--*/
{
	UNREFERENCED_PARAMETER(FltObjects);

	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,
		("!OperationStatusCallback: Entered\n"));

	PT_DBG_PRINT(PTDBG_TRACE_OPERATION_STATUS,
		("!OperationStatusCallback: Status=%08x ctx=%p IrpMj=%02x.%02x \"%s\"\n",
			OperationStatus,
			RequesterContext,
			ParameterSnapshot->MajorFunction,
			ParameterSnapshot->MinorFunction,
			FltGetIrpName(ParameterSnapshot->MajorFunction)));
}


FLT_POSTOP_CALLBACK_STATUS
PostOperation(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_opt_ PVOID CompletionContext,
	_In_ FLT_POST_OPERATION_FLAGS Flags
)
/*++

Routine Description:

	This routine is the post-operation completion routine for this
	miniFilter.

	This is non-pageable because it may be called at DPC level.

Arguments:

	Data - Pointer to the filter callbackData that is passed to us.

	FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
		opaque handles to this filter, instance, its associated volume and
		file object.

	CompletionContext - The completion context set in the pre-operation routine.

	Flags - Denotes whether the completion is successful or is being drained.

Return Value:

	The return value is the status of the operation.

--*/
{
	UNREFERENCED_PARAMETER(Data);
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);
	UNREFERENCED_PARAMETER(Flags);

	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,
		("!PostOperation: Entered\n"));

	return FLT_POSTOP_FINISHED_PROCESSING;
}


FLT_PREOP_CALLBACK_STATUS
PreOperationNoPostOperation(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID *CompletionContext
)
/*++

Routine Description:

	This routine is a pre-operation dispatch routine for this miniFilter.

	This is non-pageable because it could be called on the paging path

Arguments:

	Data - Pointer to the filter callbackData that is passed to us.

	FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
		opaque handles to this filter, instance, its associated volume and
		file object.

	CompletionContext - The context for the completion routine for this
		operation.

Return Value:

	The return value is the status of the operation.

--*/
{
	UNREFERENCED_PARAMETER(Data);
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);

	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,
		("!PreOperationNoPostOperation: Entered\n"));

	// This template code does not do anything with the callbackData, but
	// rather returns FLT_PREOP_SUCCESS_NO_CALLBACK.
	// This passes the request down to the next miniFilter in the chain.

	return FLT_PREOP_SUCCESS_NO_CALLBACK;
}


BOOLEAN
DoRequestOperationStatus(
	_In_ PFLT_CALLBACK_DATA Data
)
/*++

Routine Description:

	This identifies those operations we want the operation status for.  These
	are typically operations that return STATUS_PENDING as a normal completion
	status.

Arguments:

Return Value:

	TRUE - If we want the operation status
	FALSE - If we don't

--*/
{
	PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;

	//
	//  return boolean state based on which operations we are interested in
	//

	return (BOOLEAN)

		//
		//  Check for oplock operations
		//

		(((iopb->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL) &&
		((iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_FILTER_OPLOCK) ||
			(iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_BATCH_OPLOCK) ||
			(iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_OPLOCK_LEVEL_1) ||
			(iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_OPLOCK_LEVEL_2)))

			||

			//
			//    Check for directy change notification
			//

			((iopb->MajorFunction == IRP_MJ_DIRECTORY_CONTROL) &&
			(iopb->MinorFunction == IRP_MN_NOTIFY_CHANGE_DIRECTORY))
			);
}
#endif