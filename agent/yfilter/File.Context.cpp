#include "yfilter.h"
#include "File.h"

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