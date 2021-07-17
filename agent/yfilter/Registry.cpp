#include "yfilter.h"
#include <ntstrsafe.h>

#define USE_REGISTRY_EVENT	1

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
	g_registry.Initialize(false);
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

typedef struct _BASE_REG_KEY_INFO
{
	PVOID		pObject;
	PVOID		reserved;

} BASE_REG_KEY_INFO, *PBASE_REG_KEY_INFO;

void		RegistryDeleteValueLog(REG_CALLBACK_ARG * p, PVOID pObject, PUNICODE_STRING pValueName)
{
	UNREFERENCED_PARAMETER(p);
	UNREFERENCED_PARAMETER(pValueName);

	PUNICODE_STRING	pRegPath = NULL;
	__try {
		if (GetRegistryPath(pObject, &pRegPath)) {


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
void		CreateRegistryMessage(
	PREG_ENTRY			p,
	PY_REGISTRY_MESSAGE	*pOut
)
{
	WORD	wDataSize	= sizeof(Y_REGISTRY_DATA);
	WORD	wStringSize	= sizeof(Y_REGISTRY_STRING);
	wStringSize		+=	GetStringDataSize(&p->RegPath);
	wStringSize		+=	GetStringDataSize(&p->RegValueName);

	PY_REGISTRY_MESSAGE	pMsg	= NULL;
	HANDLE				PPID	= GetParentProcessId(p->PID);

	pMsg = (PY_REGISTRY_MESSAGE)CMemory::Allocate(PagedPool, wDataSize+wStringSize, TAG_PROCESS);
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
		pMsg->subType	= RegistryTable()->RegClass2SubType(p->nClass);
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

		pMsg->RegUID	= p->RegUID;
		pMsg->RegPUID	= p->RegPUID;
		pMsg->nCount	= p->nCount;
		pMsg->nRegDataSize	= p->dwSize;
		WORD		dwStringOffset	= (WORD)(sizeof(Y_REGISTRY_MESSAGE));

		pMsg->RegPath.wOffset	= dwStringOffset;
		pMsg->RegPath.wSize		= GetStringDataSize(&p->RegPath);
		pMsg->RegPath.pBuf		= (WCHAR *)((char *)pMsg + dwStringOffset);
		RtlStringCbCopyUnicodeString(pMsg->RegPath.pBuf, pMsg->RegPath.wSize, &p->RegPath);

		dwStringOffset			+= pMsg->RegPath.wSize;
		pMsg->RegValueName.wOffset	= dwStringOffset;
		pMsg->RegValueName.wSize	= GetStringDataSize(&p->RegValueName);
		pMsg->RegValueName.pBuf		= (WCHAR *)((char *)pMsg + dwStringOffset);
		RtlStringCbCopyUnicodeString(pMsg->RegValueName.pBuf, pMsg->RegValueName.wSize, &p->RegValueName);

		if( pOut ) {
			*pOut	= pMsg;
		}
		else {
			CMemory::Free(pMsg);
		}
	}
}
void		RegistryLog(REG_CALLBACK_ARG * p)
{
	PAGED_CODE();
	static	ULONG		nCount;
	//ULONG				nRegCount	= 0;

	if( NT_FAILED(p->status))	return;

	__try {
		if( ProcessTable()->IsExisting(p->PID, p, [](
			bool					bCreationSaved,
			IN PPROCESS_ENTRY		pEntry,			
			IN PVOID				pContext
			) {
				UNREFERENCED_PARAMETER(bCreationSaved);
				UNREFERENCED_PARAMETER(pEntry);

				PREG_CALLBACK_ARG	p	= (PREG_CALLBACK_ARG)pContext;
				p->PUID	= pEntry->PUID;
				switch( p->notifyClass ) {
					case RegNtPreDeleteKey:
						_InterlockedIncrement((LONG *)&pEntry->registry.DeleteKey.nCount);
						break;
					case RegNtPostCreateKey:
						_InterlockedIncrement((LONG *)&pEntry->registry.CreateKey.nCount);
						break;
					case RegNtRenameKey:
						_InterlockedIncrement((LONG *)&pEntry->registry.RenameKey.nCount);
						break;
					case RegNtDeleteValueKey:
						_InterlockedIncrement((LONG *)&pEntry->registry.DeleteValue.nCount);
						break;
					case RegNtPostQueryValueKey:
						_InterlockedIncrement((LONG *)&pEntry->registry.GetValue.nCount);
						break;
					case RegNtPostSetValueKey:
						_InterlockedIncrement((LONG *)&pEntry->registry.SetValue.nCount);
						break;
				}
				_InlineInterlockedAdd64(&pEntry->registry.GetValue.nSize, p->nSize);
				
				p->pContext	= NULL;
			}) ) {
				ULONG	nRegCount	= 0;
				if( p->pRegPath )
					RegistryTable()->Add(p, nRegCount);					
		}
		else {
			AddProcessToTable2(__func__, false, p->PID);
		}		
	}
	__finally {

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

	if( NULL == pArgument2 )	return STATUS_SUCCESS;

	UNREFERENCED_PARAMETER(pCallbackContext);

	if( 0 == Config()->nRun )	{
		//__log("%-32s %d", __func__, Config()->nRun);
		return STATUS_SUCCESS;
	}
	HANDLE	PID	= PsGetCurrentProcessId();
	if (NULL == pArgument2 ||
		KeGetCurrentIrql() > APC_LEVEL ||
		PID <= (HANDLE)4 ||
		KernelMode == ExGetPreviousMode()
	)	return STATUS_SUCCESS;	

	REG_CALLBACK_ARG	arg;
	RtlZeroMemory(&arg, sizeof(arg));
	arg.PID			= PID;
	arg.TID			= PsGetCurrentThreadId();
	arg.pArgument	= pArgument;
	arg.pArgument2	= pArgument2;
	arg.status		= STATUS_SUCCESS;
	arg.notifyClass	= (REG_NOTIFY_CLASS)(ULONG_PTR)pArgument;

	CFltObjectReference	filter(Config()->pFilter);
	{	
		REG_CALLBACK_ARG	*p	= (REG_CALLBACK_ARG *)&arg;			
		switch (p->notifyClass)
		{
			case RegNtPreOpenKey:

				break;
			case RegNtPreCreateKey:
			if( false )
			{
				PREG_PRE_CREATE_KEY_INFORMATION	pp	= 
					(PREG_PRE_CREATE_KEY_INFORMATION)p->pArgument2;
				p->pRegPath			= pp->CompleteName;
				p->pRegValueName	= NULL;
				p->subType			= YFilter::Message::SubType::RegistryCreateKey;
				__log("RegNtPreCreateKey %wZ", p->pRegPath);
				RegistryLog(p);		
			}
			break;

			case RegNtPostCreateKey:
			{
				PREG_POST_CREATE_KEY_INFORMATION	pp	= 
					(PREG_POST_CREATE_KEY_INFORMATION)p->pArgument2;
				PUNICODE_STRING		pRegPath;
				if( GetRegistryPath(pp->Object, &pRegPath) ) {
					p->pRegPath			= pRegPath;		
					p->pRegValueName	= pp->CompleteName;
					p->subType			= YFilter::Message::SubType::RegistryCreateKey;
					__log("RegNtPostCreateKey %wZ %wZ", p->pRegPath, p->pRegValueName);
					RegistryLog(p);
					CMemory::Free(pRegPath);
				}			
			}
			break;

			case RegNtRenameKey:
			{
				PREG_RENAME_KEY_INFORMATION 	pp	= 
					(PREG_RENAME_KEY_INFORMATION )p->pArgument2;
				PUNICODE_STRING		pRegPath;
				if( GetRegistryPath(pp->Object, &pRegPath) ) {
					p->pRegPath			= pRegPath;		
					p->pRegValueName	= pp->NewName;
					p->subType			= YFilter::Message::SubType::RegistryRenameKey;
					__log("RegNtRenameKey %wZ %wZ", p->pRegPath, pp->NewName);
					RegistryLog(p);
					CMemory::Free(pRegPath);
				}			
			
			}
			break;
			case RegNtPreDeleteKey:
			if( true )
			{
				PREG_DELETE_KEY_INFORMATION 	pp	= 
					(PREG_DELETE_KEY_INFORMATION )p->pArgument2;
				PUNICODE_STRING		pRegPath;
				if( GetRegistryPath(pp->Object, &pRegPath) ) {
					p->pRegPath			= pRegPath;		
					p->pRegValueName	= NULL;
					p->subType			= YFilter::Message::SubType::RegistryDeleteKey;
					//p->pRegValueName	= pPreInfo->ValueName;
					//p->nSize			= pPreInfo->ResultLength? *pPreInfo->ResultLength : 0;
					//__log("RegNtPreDeleteKey %wZ", p->pRegPath);
					RegistryLog(p);
					CMemory::Free(pRegPath);
				}
			}
			break;

			case RegNtPostDeleteKey:
			if( false )
			{
				PREG_POST_OPERATION_INFORMATION 	pp	= 
					(PREG_POST_OPERATION_INFORMATION )p->pArgument2;
				PREG_QUERY_VALUE_KEY_INFORMATION	pPreInfo	= 
					(PREG_QUERY_VALUE_KEY_INFORMATION)pp->PreInformation;
				PUNICODE_STRING		pRegPath;

				if( pPreInfo && GetRegistryPath(pPreInfo->Object, &pRegPath) ) {
					//	pp의 것은 이미 지워져서 경로를 읽을 수 없어요.
					p->pRegPath			= pRegPath;							
					//p->pRegValueName	= pPreInfo->ValueName;
					//p->nSize			= pPreInfo->ResultLength? *pPreInfo->ResultLength : 0;
					p->subType			= YFilter::Message::SubType::RegistryDeleteKey;
					__log("RegNtPostDeleteKey %wZ %p", p->pRegPath, pPreInfo);
					RegistryLog(p);
					CMemory::Free(pRegPath);
				}
			}
			break;

			case RegNtPreQueryValueKey:
				//if( false ) {
				//	PREG_QUERY_VALUE_KEY_INFORMATION	pp	= 
				//		(PREG_QUERY_VALUE_KEY_INFORMATION)p->pArgument2;
				//	RegistryPreQueryValueLog(p, pp);
				//}
				break;

			case RegNtPostQueryValueKey:
				if( true ) {
					//	레지스트리 읽는 것까지 모니터링 하니까 너무 부하가 많아 힘들다.
					//	이건 그냥 보내고 카운트만 하자.
					PREG_POST_OPERATION_INFORMATION 	pp	= 
						(PREG_POST_OPERATION_INFORMATION )p->pArgument2;
					PREG_QUERY_VALUE_KEY_INFORMATION	pPreInfo	= 
						(PREG_QUERY_VALUE_KEY_INFORMATION)pp->PreInformation;
						p->status		= pp->Status;
						p->nSize		= ( pPreInfo->ResultLength )? *(pPreInfo->ResultLength) : 0;
						p->subType			= YFilter::Message::SubType::RegistryGetValue;
						RegistryLog(p);
				}
				break;

			case RegNtPostSetValueKey:
				{
					PREG_POST_OPERATION_INFORMATION		pp	= 
						(PREG_POST_OPERATION_INFORMATION )p->pArgument2;
					PREG_QUERY_VALUE_KEY_INFORMATION	pPreInfo	= 
						(PREG_QUERY_VALUE_KEY_INFORMATION)pp->PreInformation;
					if( NT_SUCCESS(pp->Status) ) {
						if( pPreInfo ) {
							PUNICODE_STRING		pRegPath;
							if( GetRegistryPath(pp->Object, &pRegPath) ) {
								p->pRegPath			= pRegPath;		
								p->pRegValueName	= pPreInfo->ValueName;
								p->nSize			= pPreInfo->ResultLength? *pPreInfo->ResultLength : 0;
								p->subType			= YFilter::Message::SubType::RegistrySetValue;
								//__log("RegNtPostSetValueKey %wZ %wZ", p->pRegPath, p->pRegValueName);
								RegistryLog(p);
								CMemory::Free(pRegPath);
							}
						}
					}
				}
				break;

			case RegNtPreSetValueKey:		//	== RegNtSetValueKey 레지스트리 값 추가
			if( true ) {
				//PREG_SET_VALUE_KEY_INFORMATION	pp	= 
				//	(PREG_SET_VALUE_KEY_INFORMATION)p->pArgument2;
				//RegistryPreSetValueLog(p, pp);
			}
				break;
			case RegNtDeleteValueKey:	//	레지스트리 값 삭제
			if( true ) {
				PREG_DELETE_VALUE_KEY_INFORMATION	pp	= 
					(PREG_DELETE_VALUE_KEY_INFORMATION)p->pArgument2;
				PUNICODE_STRING		pRegPath;
				if( GetRegistryPath(pp->Object, &pRegPath) ) {
					p->pRegPath			= pRegPath;		
					p->pRegValueName	= pp->ValueName;
					p->subType			= YFilter::Message::SubType::RegistryDeleteValue;
					RegistryLog(p);
					CMemory::Free(pRegPath);
				}
			}
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