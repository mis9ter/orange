#include "yfilter.h"
#include "File.h"

#ifdef ALLOC_PRAGMA
//#pragma alloc_text(PAGE, CreateFileMessage)
#endif

void					CreateFileMessage(
	PFILE_ENTRY			p,
	PY_FILE_MESSAGE		*pOut
)
{
	WORD	wDataSize	= sizeof(Y_FILE_DATA);
	WORD	wStringSize	= sizeof(Y_FILE_STRING);
	wStringSize		+=	GetStringDataSize(&p->FilePath);

	PY_FILE_MESSAGE		pMsg	= NULL;
	HANDLE				PPID	= GetParentProcessId(p->PID);

	pMsg = (PY_FILE_MESSAGE)CMemory::Allocate(PagedPool, wDataSize+wStringSize, TAG_PROCESS);
	if (pMsg) 
	{
		//__dlog("%-32s message size:%d", __func__, wPacketSize);
		//__dlog("  header :%d", sizeof(Y_HEADER));
		//__dlog(" struct  :%d", sizeof(Y_REGISTRY));
		//__dlog(" regpath :%d", GetStringDataSize(pRegPath));
		//__dlog(" value   :%d", GetStringDataSize(pRegValueName));

		RtlZeroMemory(pMsg, wDataSize+wStringSize);

		pMsg->mode		= YFilter::Message::Mode::Event;
		pMsg->category	= YFilter::Message::Category::Registry;
		//pMsg->subType	= FileTable()->Class2SubType(p->nClass);
		pMsg->wDataSize	= wDataSize;
		pMsg->wStringSize	= wStringSize;
		pMsg->wRevision	= 1;

		pMsg->PUID		= p->PUID;
		pMsg->PID		= (DWORD)p->PID;
		pMsg->CPID		= (DWORD)p->PID;
		pMsg->RPID		= (DWORD)0;
		pMsg->PPID		= (DWORD)PPID;
		pMsg->CTID		= (DWORD)p->TID;
		pMsg->TID		= pMsg->CTID;

		pMsg->FUID		= p->FUID;
		pMsg->FPUID		= p->FPUID;
		pMsg->nCount	= p->nCount;
		pMsg->nSize		= p->nSize;
		WORD		dwStringOffset	= (WORD)(sizeof(Y_REGISTRY_MESSAGE));

		pMsg->Path.wOffset	= dwStringOffset;
		pMsg->Path.wSize	= GetStringDataSize(&p->FilePath);
		pMsg->Path.pBuf		= (WCHAR *)((char *)pMsg + dwStringOffset);
		RtlStringCbCopyUnicodeString(pMsg->Path.pBuf, pMsg->Path.wSize, &p->FilePath);
		if( pOut ) {
			*pOut	= pMsg;
		}
		else {
			CMemory::Free(pMsg);
		}
	}
}