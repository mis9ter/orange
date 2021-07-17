#include "yfilter.h"
#include "File.h"

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
		//	�̹� �ش� ���μ����� ����� ���̴�.
		//	�� ��� �ۿ��� ����� ���μ����� ���ν����ش�.
		//	�츮���� PUID�� �ִ�.
		//__log("%-32s PID:%d is not found.", __func__, (DWORD)p->PID);		
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