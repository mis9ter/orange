#include "yfilter.h"

VOID	FilterInstanceContextCleanup(
	__in PFLT_CONTEXT Context,
	__in FLT_CONTEXT_TYPE ContextType
)
{
	UNREFERENCED_PARAMETER(Context);
	UNREFERENCED_PARAMETER(ContextType);
}
VOID	FilterContextCleanup (
	__in PFLT_CONTEXT Context,
	__in FLT_CONTEXT_TYPE ContextType
)
{
	UNREFERENCED_PARAMETER(Context);
	UNREFERENCED_PARAMETER(ContextType);
}

const FLT_CONTEXT_REGISTRATION ContextRegistration[] = {

	{ FLT_STREAMHANDLE_CONTEXT,
	0,
	FilterContextCleanup,
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