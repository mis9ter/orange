#include "yfilter.h"

void		MessageThreadCallback(PQUEUE_WORKITEM p, PVOID pCallbackPtr)
{
	UNREFERENCED_PARAMETER(pCallbackPtr);
	if (p)
	{
		if ( sizeof(YFILTER_MESSAGE_DATA) == p->nSize )
		{
			PYFILTER_MESSAGE_DATA	pMsg	= (PYFILTER_MESSAGE_DATA)p->pItem;
			__log("%s [%d][%d]%ws", __FUNCTION__, pMsg->data.type, pMsg->data.bCreationSaved,
					pMsg->data.szPath);
			SendMessage("PROCESS_STOP", &Config()->client.event, pMsg, p->nSize);
		}
	}
}
void		SetMessageThreadCallback(PVOID pCallbackPtr)
{
	MessageThreadPool()->SetCallback(MessageThreadCallback, pCallbackPtr);
}