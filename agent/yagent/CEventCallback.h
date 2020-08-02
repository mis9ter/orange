#pragma once

#include "orangedb.h"
#include "yagent.string.h"
#include "CProcess.h"

#define		EVENT_DB_NAME	L"event.db"

class CEventCallback
	:
	virtual	public	CAppLog,
	public	CDB,
	public	CProcess,
	public	CPathConvertor
{
public:
	CEventCallback() :
		CDB(EVENT_DB_NAME)
	{
		m_bDbIsOpened	= IsOpened();
	}
	~CEventCallback() {

	}

	CDB* Db() {
		return dynamic_cast<CDB *>(this);
	}
	CPathConvertor* PathConvertor() {
		return dynamic_cast<CPathConvertor *>(this);
	}
protected:

	static	bool			EventCallbackProc(
		ULONGLONG			nMessageId,
		PVOID				pCallbackPtr,
		int					nCategory,
		int					nSubType,
		PVOID				pData,
		size_t				nSize
	) {
		CEventCallback				*pClass	= (CEventCallback *)pCallbackPtr;		

		if( YFilter::Message::Category::Process == nCategory ) {
			PYFILTER_DATA	p = (PYFILTER_DATA)pData;
			return ProcessCallbackProc(nMessageId, dynamic_cast<CProcess *>(pClass), p);
		}
		return true;
	}
private:
	bool		m_bDbIsOpened;

};
