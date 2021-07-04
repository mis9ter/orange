#include "yfilter.h"

VOID	FilterInstanceContextCleanup(
	__in PFLT_CONTEXT Context,
	__in FLT_CONTEXT_TYPE ContextType
)
{
	UNREFERENCED_PARAMETER(Context);
	UNREFERENCED_PARAMETER(ContextType);
}
void	DeleteStreamHandleCallback(PY_STREAMHANDLE_CONTEXT p);
VOID	FilterStreamHandleContextCleanup (
	__in PFLT_CONTEXT Context,
	__in FLT_CONTEXT_TYPE ContextType
)
{
	UNREFERENCED_PARAMETER(Context);
	UNREFERENCED_PARAMETER(ContextType);

	//__dlog("%-32s", __func__);

	PY_STREAMHANDLE_CONTEXT	p	= (PY_STREAMHANDLE_CONTEXT)Context;

	__try {
		if( FLT_STREAMHANDLE_CONTEXT == ContextType ) {
			DeleteStreamHandleCallback(p);
		}
	}
	__finally {
	
	
	}
}

const FLT_CONTEXT_REGISTRATION ContextRegistration[] = {

	{ FLT_STREAMHANDLE_CONTEXT,
	0,
	FilterStreamHandleContextCleanup,
	sizeof(Y_STREAMHANDLE_CONTEXT),
	'XCnG' 
	},
	{ 
		FLT_INSTANCE_CONTEXT,
		0,
		FilterInstanceContextCleanup,
		sizeof(Y_INSTANCE_CONTEXT),
		'XCnG' 
	},
	{ FLT_CONTEXT_END }
};