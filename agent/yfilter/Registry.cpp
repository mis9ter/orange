#include "yfilter.h"
#include <ntstrsafe.h>

#define USE_REGISTRY_EVENT	0

static	LARGE_INTEGER	g_nCookie;
static	CRegTable		g_registry;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(NONPAGE, RegistryTable)
#endif

CRegTable *	RegistryTable()
{
	return &g_registry;
}
void			CreateRegistryTable()
{
	g_registry.Initialize();
}
void			DestroyRegistryTable()
{
	g_registry.Destroy();
}

BOOLEAN	NTAPI	GetRegistryPath
(
	IN	PVOID				pObject,
	OUT	PUNICODE_STRING *	pObjectName
)
{
	bool			bRet	= false;
	NTSTATUS		status	= STATUS_SUCCESS;
	ULONG			nSize	= AGENT_PATH_SIZE * sizeof(WCHAR);
	PUNICODE_STRING	pName	= NULL;
	KIRQL			irql	= KeGetCurrentIrql();

	__try
	{
		if (NULL == pObject || NULL == pObjectName)	__leave;
		if( irql >= DISPATCH_LEVEL ) {
			__dlog("%-32s irql(%d) is too high", __func__, irql);
			__leave;		
		}
		pName = (PUNICODE_STRING)CMemory::Allocate(PagedPool, nSize + sizeof(UNICODE_STRING), 'GRGP'); 		
		if (NULL == pName) {
			__log("%s memory allocation failed. %d bytes", __FUNCTION__, nSize + sizeof(UNICODE_STRING));
			__leave;
		}
		RtlZeroMemory(pName, nSize);
		pName->Length = (USHORT)nSize - sizeof(WCHAR);
		pName->MaximumLength = (USHORT)nSize;
		if( pName->Length ) {
			/*
				ObQueryNameString	이 함수의 재미있는 특징
				pName->Length의 값이 0이면 BSOD

				MSDN Remarks:
				If the given object is named and the object name was successfully acquired,
				the returned string is the name of the given object including as much of the object's full path as possible.
				In this case, ObQueryNameString sets
				Name.Buffer to the address of the NULL-terminated name of the specified object.
				The value of Name.MaximumLength is the length of the object name including the NULL termination.
				The value of Name.Length is length of the only the object name.
			*/
			status = ObQueryNameString(pObject, (POBJECT_NAME_INFORMATION)pName, nSize, &nSize);
			if (NT_FAILED(status))
			{
				__log("ObQueryNameString() failed. status=%08x", status);
				__leave;
			}
			bRet = true;
		}
		else 
			status	= STATUS_UNSUCCESSFUL;
	}
	__finally
	{
		if (bRet) {
			if (pObjectName)	*pObjectName = pName;
		}
		else {
			if (pName) 
			{
				CMemory::Free(pName);
			}
		}
	}
	return bRet;
}
static	CCRC64		g_crc64;

PROCUID		Path2CRC64(PUNICODE_STRING pValue) {
	if( NULL == pValue || 0 == pValue->Length )	return 0;
	return g_crc64.GetCrc64(pValue->Buffer, pValue->Length);
}
PROCUID		Path2CRC64(PCWSTR pValue) {
	if( NULL == pValue )	return 0;
	return g_crc64.GetCrc64((PVOID)pValue, (DWORD)(wcslen(pValue) * sizeof(WCHAR)));
}
typedef struct {
	PVOID				pArgument2;	
	HANDLE				PID;
	REGUID				RegUID;
	PUNICODE_STRING		pRegKeyPath;
	PVOID				pArgument;
	REG_NOTIFY_CLASS	notifyClass;
	NTSTATUS			status;
	PVOID				pContext;
}	REG_CALLBACK_ARG, *PREG_CALLBACK_ARG;
typedef struct _BASE_REG_KEY_INFO
{
	PVOID		pObject;
	PVOID		reserved;

} BASE_REG_KEY_INFO, *PBASE_REG_KEY_INFO;

void		RegistryDeleteValueLog(REG_CALLBACK_ARG * p, PVOID pObject, PUNICODE_STRING pValueName)
{
	PUNICODE_STRING	pRegPath = NULL;
	__try {
		if (GetRegistryPath(pObject, &pRegPath)) {		
			REGUID	RegUID	= Path2CRC64(pRegPath);

			bool	bAdd	= RegistryTable()->Add(RegUID, pRegPath, p, 
				[](PREG_ENTRY pEntry, PVOID pContext) {
			
					UNREFERENCED_PARAMETER(pEntry);
					UNREFERENCED_PARAMETER(pContext);
			
			});
			if( bAdd ) {			
				__log("ADD %d %I64d %wZ %wZ", p->notifyClass, RegUID, pRegPath, pValueName);			
			}
		}		
		else {
			__log("%-32s GetRegistryPath() failed.", __func__);
		}
	}
	__finally {
		if (pRegPath)
		{
			CMemory::Free(pRegPath);
		}	
	}
	/*
		switch (notifyClass)
		{
			case RegNtQueryValueKey:
				pEntry->registry.value.dwRead++;
				break;

			case RegNtPostSetValueKey:
				pEntry->registry.value.dwWrite++;
				break;

			case RegNtPreSetValueKey:		//	레지스트리 값 추가
											//RegistryLog("RegNtSetValueKey", PID, ((PBASE_REG_KEY_INFO)pArgument2)->pObject);
				break;
			case RegNtDeleteValueKey:	//	레지스트리 값 삭제
			{
				PREG_DELETE_VALUE_KEY_INFORMATION	pp	= 
					(PREG_DELETE_VALUE_KEY_INFORMATION)p->pArgument2;
				RegistryLog("RegNtDeleteValueKey", p->PID, pp->Object, pp->ValueName);
				pEntry->registry.value.dwDelete++;
			}
			break;

			case RegNtRenameKey:
				//RegistryLog("RegNtRenameKey", PID, ((PBASE_REG_KEY_INFO)pArgument2)->pObject);
				pEntry->registry.key.dwRename++;
				break;

			case RegNtPreCreateKey:
				RegistryLog("RegNtPreCreateKey", p->PID, ((PBASE_REG_KEY_INFO)p->pArgument2)->pObject, NULL);
				pEntry->registry.key.dwCreate++;
				break;
			case RegNtDeleteKey:
				//RegistryLog("RegNtDeleteKey", PID, ((PBASE_REG_KEY_INFO)pArgument2)->pObject);
				pEntry->registry.key.dwDelete++;
				break;
			case RegNtPreSetKeySecurity:

				break;
		}
	*/
}
void		CreateRegistryContext(

	PROCUID * pPUID, 
	REGUID * pRegUID, 
	YFilter::Message::SubType	subType,
	PUNICODE_STRING pRegPath, 
	PUNICODE_STRING pRegValueName,
	PVOID	*pOut
)
{
	WORD				wPacketSize	= sizeof(Y_HEADER);

	wPacketSize		+=	sizeof(Y_REGISTRY);
	wPacketSize		+=	GetStringDataSize(pRegPath);
	wPacketSize		+=	GetStringDataSize(pRegValueName);

	PY_REGISTRY			pMsg	= NULL;
	HANDLE				PID		= PsGetCurrentProcessId();
	HANDLE				PPID	= GetParentProcessId(PID);

	pMsg = (PY_REGISTRY)CMemory::Allocate(PagedPool, wPacketSize, TAG_PROCESS);
	if (pMsg) 
	{
		//__dlog("%-32s message size:%d", __func__, wPacketSize);
		//__dlog("  header :%d", sizeof(Y_HEADER));
		//__dlog(" struct  :%d", sizeof(Y_REGISTRY));
		//__dlog(" regpath :%d", GetStringDataSize(pRegPath));
		//__dlog(" value   :%d", GetStringDataSize(pRegValueName));

		pMsg->mode		= YFilter::Message::Mode::Event;
		pMsg->category	= YFilter::Message::Category::Registry;
		pMsg->subType	= subType;
		pMsg->wSize		= wPacketSize;
		pMsg->wRevision	= 1;

		pMsg->PUID		= *pPUID;
		pMsg->PID		= (DWORD)PID;
		pMsg->CPID		= (DWORD)PID;
		pMsg->RPID		= (DWORD)0;
		pMsg->PPID		= (DWORD)PPID;
		pMsg->CTID		= (DWORD)PsGetCurrentThreadId();
		pMsg->TID		= pMsg->CTID;

		pMsg->RegUID	= *pRegUID;

		WORD		dwStringOffset	= (WORD)(sizeof(Y_HEADER) + sizeof(Y_REGISTRY));

		pMsg->RegPath.wOffset	= dwStringOffset;
		pMsg->RegPath.wSize		= GetStringDataSize(pRegPath);
		pMsg->RegPath.pBuf		= (WCHAR *)((char *)pMsg + dwStringOffset);
		RtlStringCbCopyUnicodeString(pMsg->RegPath.pBuf, pMsg->RegPath.wSize, pRegPath);

		dwStringOffset			+= pMsg->RegPath.wSize;
		pMsg->RegValueName.wOffset	= dwStringOffset;
		pMsg->RegValueName.wSize	= GetStringDataSize(pRegValueName);
		pMsg->RegValueName.pBuf		= (WCHAR *)((char *)pMsg + dwStringOffset);
		RtlStringCbCopyUnicodeString(pMsg->RegValueName.pBuf, pMsg->RegValueName.wSize, pRegValueName);

		if( pOut ) {
			*pOut	= pMsg;
		}
		else {
			CMemory::Free(pMsg);
		}
	}
}

void		RegistryPostQueryValueLog(REG_CALLBACK_ARG * p, PREG_POST_OPERATION_INFORMATION  pp)
{
	PAGED_CODE();
	UNREFERENCED_PARAMETER(p);
	PUNICODE_STRING	pRegPath = NULL;

	if( NT_FAILED(pp->Status) )	return;

	PREG_QUERY_VALUE_KEY_INFORMATION	pPreInfo	= (PREG_QUERY_VALUE_KEY_INFORMATION)pp->PreInformation;

	__try {
		if (GetRegistryPath(pp->Object, &pRegPath)) {		
			p->RegUID		= Path2CRC64(pRegPath);
			p->pRegKeyPath	= pRegPath;

			if( ProcessTable()->IsExisting(p->PID, p, [](
				bool					bCreationSaved,
				IN PPROCESS_ENTRY		pEntry,			
				IN PVOID				pContext
				) {
				UNREFERENCED_PARAMETER(bCreationSaved);
				UNREFERENCED_PARAMETER(pEntry);

				PREG_CALLBACK_ARG					p	= (PREG_CALLBACK_ARG)pContext;
				PREG_POST_OPERATION_INFORMATION		pp	= 
					(PREG_POST_OPERATION_INFORMATION)(p->pArgument2);
				PREG_QUERY_VALUE_KEY_INFORMATION	pPreInfo	= (PREG_QUERY_VALUE_KEY_INFORMATION)pp->PreInformation;

				ULONG	nSize	= 0;
				if( pPreInfo ) {
					if( pPreInfo->ResultLength )
						nSize	= *(pPreInfo->ResultLength);
					_InterlockedIncrement((LONG *)&pEntry->registry.GetValue.nCount);
					_InlineInterlockedAdd64(&pEntry->registry.GetValue.nSize, nSize);
					CreateRegistryContext(&pEntry->PUID, &p->RegUID, 
						YFilter::Message::SubType::RegistryGetValue,
						p->pRegKeyPath, pPreInfo->ValueName, &p->pContext);
				}
			}
			) ) {
				if( p->pContext ) {
					PY_REGISTRY		pReg	= (PY_REGISTRY)p->pContext;
					//__log("%ws[%d]", pReg->RegPath.pBuf, pReg->RegPath.wSize);
					//__log("%ws[%d]", pReg->RegValueName.pBuf, pReg->RegValueName.wSize);
					if( pPreInfo->ResultLength )
						pReg->nDataSize	= *(pPreInfo->ResultLength);
					if (MessageThreadPool()->Push(__FUNCTION__,
						pReg->mode,
						pReg->category,
						pReg, pReg->wSize, false))
					{
						//	pMsg는 SendMessage 성공 후 해제될 것이다. 
						MessageThreadPool()->Alert(YFilter::Message::Category::Registry);
						p->pContext	= NULL;
					}
					else {
						__log("%-32s Push() failed.", __func__);

					}
					if( p->pContext ) {
						CMemory::Free(p->pContext);
						p->pContext	= NULL;
					}
				}
			}
			else {
				__dlog("%-32s PID(%d) is not found.", __func__, p->PID);

			}		
		}		
		else {
			__log("%-32s GetRegistryPath() failed.", __func__);
		}
	}
	__finally {
		if (pRegPath)
		{
			CMemory::Free(pRegPath);
			p->pRegKeyPath	= NULL;
		}	
	}
}
void		RegistryGetValueLog(REG_CALLBACK_ARG * p, PREG_QUERY_VALUE_KEY_INFORMATION pp)
{
	PAGED_CODE();
	UNREFERENCED_PARAMETER(p);
	PUNICODE_STRING	pRegPath = NULL;
	__try {
		if (GetRegistryPath(pp->Object, &pRegPath)) {		
			p->RegUID		= Path2CRC64(pRegPath);
			p->pRegKeyPath	= pRegPath;

			if( ProcessTable()->IsExisting(p->PID, p, [](
				bool					bCreationSaved,
				IN PPROCESS_ENTRY		pEntry,			
				IN PVOID				pContext
				) {
				UNREFERENCED_PARAMETER(bCreationSaved);
				UNREFERENCED_PARAMETER(pEntry);

				PREG_CALLBACK_ARG				p	= (PREG_CALLBACK_ARG)pContext;
				PREG_SET_VALUE_KEY_INFORMATION	pp	= 
					(PREG_SET_VALUE_KEY_INFORMATION)(p->pArgument2);

				_InterlockedIncrement((LONG *)&pEntry->registry.GetValue.nCount);
				_InlineInterlockedAdd64(&pEntry->registry.GetValue.nSize, pp->DataSize);
				CreateRegistryContext(&pEntry->PUID, &p->RegUID, 
					YFilter::Message::SubType::RegistryGetValue,
					p->pRegKeyPath, pp->ValueName, &p->pContext);
			}
			) ) {
				if( p->pContext ) {
					PY_REGISTRY		pReg	= (PY_REGISTRY)p->pContext;
					//__log("%ws[%d]", pReg->RegPath.pBuf, pReg->RegPath.wSize);
					//__log("%ws[%d]", pReg->RegValueName.pBuf, pReg->RegValueName.wSize);
					if( pp->ResultLength )
						pReg->nDataSize	= *pp->ResultLength;
					if (MessageThreadPool()->Push(__FUNCTION__,
						pReg->mode,
						pReg->category,
						pReg, pReg->wSize, false))
					{
						//	pMsg는 SendMessage 성공 후 해제될 것이다. 
						MessageThreadPool()->Alert(YFilter::Message::Category::Registry);
						p->pContext	= NULL;
					}
					else {
						__log("%-32s Push() failed.", __func__);

					}
					if( p->pContext ) {
						CMemory::Free(p->pContext);
						p->pContext	= NULL;
					}
				}
			}
			else {
				__dlog("%-32s PID(%d) is not found.", __func__, p->PID);

			}		
		}		
		else {
			__log("%-32s GetRegistryPath() failed.", __func__);
		}
	}
	__finally {
		if (pRegPath)
		{
			CMemory::Free(pRegPath);
			p->pRegKeyPath	= NULL;
		}	
	}
}
void		RegistrySetValueLog(REG_CALLBACK_ARG * p, PREG_SET_VALUE_KEY_INFORMATION pp)
{
	PAGED_CODE();
	UNREFERENCED_PARAMETER(p);
	PUNICODE_STRING	pRegPath = NULL;
	__try {
		if (GetRegistryPath(pp->Object, &pRegPath)) {		
			p->RegUID		= Path2CRC64(pRegPath);
			p->pRegKeyPath	= pRegPath;
			/*
			bool	bAdd	= RegistryTable()->Add(RegUID, pRegPath, p, 
				[](PREG_ENTRY pEntry, PVOID pContext) {

				UNREFERENCED_PARAMETER(pEntry);
				UNREFERENCED_PARAMETER(pContext);

			});
			if( bAdd ) {			
				__log("ADD %d %I64d %wZ %wZ", p->notifyClass, RegUID, pRegPath, pp->ValueName);			
			}
			*/
			if( ProcessTable()->IsExisting(p->PID, p, [](
				bool					bCreationSaved,
				IN PPROCESS_ENTRY		pEntry,			
				IN PVOID				pContext
				) {
					UNREFERENCED_PARAMETER(bCreationSaved);
					UNREFERENCED_PARAMETER(pEntry);

					PREG_CALLBACK_ARG				p	= (PREG_CALLBACK_ARG)pContext;
					PREG_SET_VALUE_KEY_INFORMATION	pp	= 
						(PREG_SET_VALUE_KEY_INFORMATION)(p->pArgument2);

					_InterlockedIncrement((LONG *)&pEntry->registry.SetValue.nCount);
					_InlineInterlockedAdd64(&pEntry->registry.SetValue.nSize, pp->DataSize);
					CreateRegistryContext(&pEntry->PUID, &p->RegUID, 
						YFilter::Message::SubType::RegistrySetValue,
						p->pRegKeyPath, pp->ValueName, &p->pContext);
				}
			) ) {
				if( p->pContext ) {
					PY_REGISTRY		pReg	= (PY_REGISTRY)p->pContext;
					//__log("%ws[%d]", pReg->RegPath.pBuf, pReg->RegPath.wSize);
					//__log("%ws[%d]", pReg->RegValueName.pBuf, pReg->RegValueName.wSize);

					//__log("%-32s message size:%d", __func__, pReg->wSize);
					//__log("  header :%d", sizeof(Y_HEADER));
					//__log(" struct  :%d", sizeof(Y_REGISTRY));
					//__log(" regpath :%ws[%d]", pReg->RegPath.pBuf, pReg->RegPath.wSize);
					//__log(" value   :%ws[%d]", pReg->RegValueName.pBuf, pReg->RegValueName.wSize);

					pReg->nDataSize	= pp->DataSize;
					if (MessageThreadPool()->Push(__FUNCTION__,
						pReg->mode,
						pReg->category,
						pReg, pReg->wSize, false))
					{
						//	pMsg는 SendMessage 성공 후 해제될 것이다. 
						MessageThreadPool()->Alert(YFilter::Message::Category::Registry);
						p->pContext	= NULL;
					}
					else {
						__log("%-32s Push() failed.", __func__);
					}
					if( p->pContext ) {
						CMemory::Free(p->pContext);
						p->pContext	= NULL;
					}
				}
			}
			else {
				__dlog("%-32s PID(%d) is not found.", __func__, p->PID);
				if( false == AddProcessToTable(__func__, false, p->PID) ) {
					__log("%-32s AddProcessToTable(%d) failed.", __func__, p->PID);
				
				}
			}		

#if defined(USE_REGISTRY_EVENT) && 1 == USE_REGISTRY_EVENT 

			if( Config()->client.event.nConnected > 0 ) {
			
				PYFILTER_MESSAGE	pMsg	= NULL;
				PUNICODE_STRING		pImageFileName = NULL;
				HANDLE				PID		= PsGetCurrentProcessId();
				HANDLE				PPID	= GetParentProcessId(PID);
				if (NT_SUCCESS(GetProcessImagePathByProcessId(PID, &pImageFileName)))
				{
					pMsg = (PYFILTER_MESSAGE)CMemory::Allocate(PagedPool, sizeof(YFILTER_MESSAGE), TAG_PROCESS);
					if (pMsg) 
					{
						PROCUID				PUID;
						KERNEL_USER_TIMES	times;

						__log("%-32s message size:%d", __func__, sizeof(YFILTER_MESSAGE));

						RtlZeroMemory(pMsg, sizeof(YFILTER_MESSAGE));
						RtlStringCbCopyUnicodeString(pMsg->data.szPath, sizeof(pMsg->data.szPath), pRegPath);
						if( NT_SUCCESS(GetProcGuid(true, PID, PPID,
							pImageFileName, &times.CreateTime, &pMsg->data.ProcGuid, &PUID)) ) 
						{
							pMsg->data.PUID	= PUID;
							PUNICODE_STRING	pParentImageFileName = NULL;
							if (NT_SUCCESS(GetProcessImagePathByProcessId(PPID, &pParentImageFileName)))
							{
								KERNEL_USER_TIMES	ptimes;
								PROCUID				PPUID;
								if (NT_SUCCESS(GetProcessTimes(PPID, &ptimes))) {
									if (NT_SUCCESS(GetProcGuid(true, PPID, GetParentProcessId(PPID),
										pParentImageFileName, &ptimes.CreateTime, &pMsg->data.PProcGuid, &PPUID))) {
										pMsg->data.PPUID	= PPUID;
									}
									else {
										__log("%s PProcGuid - failed.", __FUNCTION__);
										__log("  PPID:%d", PPID);
										__log("  GetProcGuid() failed.");
									}
								}
								else {
									__log("%s PProcGuid - failed.", __FUNCTION__);
									__log("  PPID:%d", PPID);
									__log("  GetProcessTimes() failed.");
								}
								CMemory::Free(pParentImageFileName);
							}	
						
							pMsg->data.times.CreateTime = times.CreateTime;
							pMsg->header.mode			= YFilter::Message::Mode::Event;
							pMsg->header.category		= YFilter::Message::Category::Registry;
							pMsg->header.wSize			= sizeof(YFILTER_MESSAGE);

							pMsg->data.subType			= YFilter::Message::SubType::ProcessStart2;
							pMsg->data.bCreationSaved	= true;
							pMsg->data.PID				= (DWORD)PID;
							pMsg->data.PPID				= (DWORD)PPID;
							RtlStringCbCopyUnicodeString(pMsg->data.szPath, sizeof(pMsg->data.szPath), pRegPath);
							if (MessageThreadPool()->Push(__FUNCTION__,
								pMsg->header.mode,
								pMsg->header.category,
								pMsg, sizeof(YFILTER_MESSAGE), false))
							{
								//	pMsg는 SendMessage 성공 후 해제될 것이다. 
								MessageThreadPool()->Alert(YFilter::Message::Category::Registry);
								pMsg	= NULL;

								//__log("%-32s %wZ", __func__, pRegPath);
							}
							else {
								CMemory::Free(pMsg);
								pMsg	= NULL;
							}
						}
						else {
							__log("%-32s GetProcGuid() failed.", __func__);
					
						}
						if( pMsg ) {
							CMemory::Free(pMsg);
						}
					}
					CMemory::Free(pImageFileName);
				}
				else {
					__log("%-32s GetProcessImagePathByProcessId() failed.", __func__);
			
				}
			}
			else {
				//__log("%-32s not connected", __func__);
			
			}
#endif
		}		
		else {
			__log("%-32s GetRegistryPath() failed.", __func__);
		}
	}
	__finally {
		if (pRegPath)
		{
			CMemory::Free(pRegPath);
		}	
	}
}
NTSTATUS	RegistryCallback
(
	IN	PVOID	pCallbackContext,
	IN	PVOID	pArgument,
	IN	PVOID	pArgument2
)
{
	PAGED_CODE();

	UNREFERENCED_PARAMETER(pCallbackContext);
	UNREFERENCED_PARAMETER(pArgument2);

	if (NULL == pArgument2 ||
		KeGetCurrentIrql() > APC_LEVEL ||
		PsGetCurrentProcessId() <= (HANDLE)4 ||
		KernelMode == ExGetPreviousMode()
	)	return STATUS_SUCCESS;
	if( 0 == Config()->client.event.nConnected )
		return STATUS_SUCCESS;


	REG_CALLBACK_ARG	arg;

	arg.pArgument	= pArgument;
	arg.pArgument2	= pArgument2;
	arg.status		= STATUS_SUCCESS;
	arg.notifyClass	= (REG_NOTIFY_CLASS)(ULONG_PTR)pArgument;
	arg.RegUID		= 0;
	arg.pRegKeyPath	= NULL;
	arg.pContext	= NULL;

	//__log("%-32s pArgument2=%p, KeGetCurrentIrql()=%d", __FUNCTION__, pArgument2, KeGetCurrentIrql());



	CFltObjectReference	filter(Config()->pFilter);
	//kvnif (!filter.Reference())	return status;

	arg.PID	= PsGetCurrentProcessId();
	{	
		REG_CALLBACK_ARG	*p	= (REG_CALLBACK_ARG *)&arg;			
		switch (p->notifyClass)
		{
			case RegNtPreQueryValueKey:
				if( false ) {
					PREG_QUERY_VALUE_KEY_INFORMATION	pp	= 
						(PREG_QUERY_VALUE_KEY_INFORMATION)p->pArgument2;
					RegistryGetValueLog(p, pp);
				}
				break;

			case RegNtPostQueryValueKey:
				if( true ) {
					PREG_POST_OPERATION_INFORMATION 	pp	= 
						(PREG_POST_OPERATION_INFORMATION )p->pArgument2;
					RegistryPostQueryValueLog(p, pp);
				}
				break;

			case RegNtPostSetValueKey:
				break;

			case RegNtPreSetValueKey:		//	레지스트리 값 추가
			if( true ) {
				PREG_SET_VALUE_KEY_INFORMATION	pp	= 
					(PREG_SET_VALUE_KEY_INFORMATION)p->pArgument2;
				RegistrySetValueLog(p, pp);
			}
				break;
			case RegNtDeleteValueKey:	//	레지스트리 값 삭제
			if( false ) {
				PREG_DELETE_VALUE_KEY_INFORMATION	pp	= 
					(PREG_DELETE_VALUE_KEY_INFORMATION)p->pArgument2;
				RegistryDeleteValueLog(p, pp->Object, pp->ValueName);
			}
			break;

			case RegNtRenameKey:
				break;

			case RegNtPreCreateKey:
				break;

			case RegNtDeleteKey:
				break;
			case RegNtPreSetKeySecurity:

				break;
		}
	}
	if( ProcessTable()->IsExisting(arg.PID, &arg, 
		[](bool bCreationSaved, PPROCESS_ENTRY pEntry, PVOID pContext) {	
			UNREFERENCED_PARAMETER(bCreationSaved);
			UNREFERENCED_PARAMETER(pEntry);
			UNREFERENCED_PARAMETER(pContext);	
			//__dlog("%-32s irql=%d", __func__, KeGetCurrentIrql());
		})) {	
		//__log("known process  :%d", PID);
	}
	else {
		//__log("unknown process:%d notifyClass:%d", arg.PID, arg.notifyClass);
		//	이 경우는 해당 프로세스가 종료되면서 열어둔 키 값이 닫혀지는 것이다.
		//	아마도 코드상에서 명확히 레지스트리 키를 닫지 않은 것이다?
		//	레지스트리 핸들 릭??
		//	RegNtKeyHandleClose
		//	RegNtPostKeyHandleClose	

	}
	return arg.status;
}
bool	StartRegistryFilter(IN PDRIVER_OBJECT pDriverObject)
{
	bool	bRet = false;

	__log("%s", __FUNCTION__);
	UNREFERENCED_PARAMETER(pDriverObject);
	NTSTATUS		status = STATUS_SUCCESS;
	__try
	{
		LARGE_INTEGER		lTickCount = { 0, };
		WCHAR				szAltitude[40] = L"";
		UNICODE_STRING		altitude;

		KeQueryTickCount(&lTickCount);
		RtlStringCbPrintfW(szAltitude, sizeof(szAltitude), L"%ws.%d", DRIVER_STRING_ALTITUDE, lTickCount.LowPart);
		RtlInitUnicodeString(&altitude, szAltitude);

		g_nCookie.QuadPart	= 0;
		status = CmRegisterCallbackEx(RegistryCallback, &altitude, pDriverObject, NULL, &g_nCookie, NULL);
		//status	= CmRegisterCallback((PEX_CALLBACK_FUNCTION)RegistryCallback, NULL, &g_nCookie);
		if (NT_FAILED(status))	{
			__log("%-32s CmRegisterCallbackEx() failed. status=%08x", __FUNCTION__, status);
			
			__leave;
		}
		bRet = true;
	}
	__finally
	{

	}
	return bRet;
}

bool	StopRegistryFilter()
{
	__log("%s", __FUNCTION__);
	if (g_nCookie.QuadPart)
	{
		NTSTATUS	status = CmUnRegisterCallback(g_nCookie);
		if (NT_SUCCESS(status))
		{
			g_nCookie.QuadPart = 0;
			return true;
		}
	}
	return false;
}