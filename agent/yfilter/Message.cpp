#include "yfilter.h"

bool	SendMessage(IN YFilter::Message::Category category, 
			IN PVOID pSendData, IN ULONG nSendDataSize, 
			OUT PVOID pRecvData, OUT ULONG * pnRecvDataSize)
{
	bool	bRet	= false;
	if (NULL == Config())
	{
		//	[TODO]	Config() 함수의 동기화는 누가 시켜주지?
		return false;
	}
	if (NULL == Config()->pFilter)
	{

	}
	CFltObjectReference		filter(Config()->pFilter);
	if (filter.Reference())
	{
		NTSTATUS		status;
		LARGE_INTEGER	timeout;
		__try
		{
			if (NULL == Config()->client.pPort) {
				__leave;
			}
			status	= FltSendMessage(Config()->pFilter, 
						(YFilter::Message::Category::Command == category)?
							&Config()->server.pCommand : &Config()->server.pEvent,
						pSendData, nSendDataSize, 
						pRecvData, pnRecvDataSize,
						&timeout);
			if (STATUS_SUCCESS == status)
			{
				bRet	= true;
				__leave;
			}
			__log("%s STATUS=%x", __FUNCTION__, status);
		}
		__finally
		{
		
		}
	}
	return bRet;
}