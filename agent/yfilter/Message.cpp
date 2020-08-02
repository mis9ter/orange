#include "yfilter.h"

void		MessageThreadCallback(PQUEUE_WORKITEM p, PVOID pCallbackPtr)
{
	UNREFERENCED_PARAMETER(pCallbackPtr);
	if (p)
	{
		//__log("%s [category:%d][bCreationSaved:%d][%ws]", 
		//		__FUNCTION__, 
		//		pMsg->header.category, pMsg->data.bCreationSaved,
		//		pMsg->data.szPath);
		//PYFILTER_MESSAGE	pMsg	= (PYFILTER_MESSAGE)p->pItem;
		//__log("%s mode:%d category:%d subtype:%d", 
		//	__FUNCTION__, p->nMode, pMsg->header.category, pMsg->data.subType);
		if( YFilter::Message::Mode::Event == p->nMode )
			SendMessage(__FUNCTION__, &Config()->client.event, p->pItem, p->nSize);
		else {
			__log("unknown mode %d", p->nMode);
		}
	}
}
void		SetMessageThreadCallback(PVOID pCallbackPtr)
{
	MessageThreadPool()->SetCallback(MessageThreadCallback, pCallbackPtr);
}