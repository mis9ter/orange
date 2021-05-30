#include "yfilter.h"

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
	PVOID				pArgument;
	REG_NOTIFY_CLASS	notifyClass;
	NTSTATUS			status;
}	REG_CALLBACK_ARG;
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
void		RegistrySetValueLog(REG_CALLBACK_ARG * p, PREG_SET_VALUE_KEY_INFORMATION pp)
{
	UNREFERENCED_PARAMETER(p);
	PUNICODE_STRING	pRegPath = NULL;
	__try {
		if (GetRegistryPath(pp->Object, &pRegPath)) {		
			REGUID	RegUID	= Path2CRC64(pRegPath);
			UNREFERENCED_PARAMETER(RegUID);

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
			/*
			PYFILTER_MESSAGE	pMsg = NULL;
			pMsg = (PYFILTER_MESSAGE)CMemory::Allocate(NonPagedPoolNx, sizeof(YFILTER_MESSAGE), TAG_PROCESS);
			if (pMsg) 
			{
				PROCUID		ProcUID;
				RtlZeroMemory(pMsg, sizeof(YFILTER_MESSAGE));
				RtlStringCbCopyUnicodeString(pMsg->data.szPath, sizeof(pMsg->data.szPath), pImageFileName);
				if( NT_SUCCESS(GetProcGuid(true, hPID, hPPID,
					pImageFileName, &times.CreateTime, &pMsg->data.ProcGuid, &ProcUID)) ) 
				{
					pMsg->data.ProcUID	= ProcUID;
					PUNICODE_STRING	pParentImageFileName = NULL;
					if (NT_SUCCESS(GetProcessImagePathByProcessId(hPPID, &pParentImageFileName)))
					{
						KERNEL_USER_TIMES	ptimes;
						PROCUID				PProcUID;
						if (NT_SUCCESS(GetProcessTimes(hPPID, &ptimes))) {
							if (NT_SUCCESS(GetProcGuid(true, hPPID, GetParentProcessId(hPPID),
								pParentImageFileName, &ptimes.CreateTime, &pMsg->data.PProcGuid, &PProcUID))) {
								pMsg->data.PProcUID	= PProcUID;
							}
							else {
								__log("%s PProcGuid - failed.", __FUNCTION__);
								__log("  PPID:%d", hPPID);
								__log("  GetProcGuid() failed.");
							}
						}
						else {
							__log("%s PProcGuid - failed.", __FUNCTION__);
							__log("  PPID:%d", hPPID);
							__log("  GetProcessTimes() failed.");
						}
						CMemory::Free(pParentImageFileName);
					}
					else {
						//	부모 프로세스가 종료된 상태라면 당연히 여기로.
						//__log("%s PProcGuid - failed.", __FUNCTION__);
						//__log("  PPID:%d", hPPID);
						//__log("  GetProcessImagePathByProcessId() failed.");
					}										
				}
				else {
					//	ProcGuid 생성 실패 -> 저장할 필요 없습니다.
					__leave;
				}
				pMsg->data.times.CreateTime = times.CreateTime;
				ProcessTable()->Add(true, hPID, hPPID,
					&pMsg->data.ProcGuid, pMsg->data.ProcUID, pImageFileName, pCmdLine);
				pMsg->header.mode	= YFilter::Message::Mode::Event;
				pMsg->header.category = YFilter::Message::Category::Process;
				pMsg->header.size = sizeof(YFILTER_MESSAGE);

				pMsg->data.subType = YFilter::Message::SubType::ProcessStart2;
				pMsg->data.bCreationSaved	= true;
				pMsg->data.PID				= (DWORD)hPID;
				pMsg->data.PPID				= (DWORD)hPPID;
				RtlStringCbCopyUnicodeString(pMsg->data.szPath, sizeof(pMsg->data.szPath), pImageFileName);
				RtlStringCbCopyUnicodeString(pMsg->data.szCommand, sizeof(pMsg->data.szCommand), pCmdLine);
				if (MessageThreadPool()->Push(__FUNCTION__,
					YFilter::Message::Mode::Event,
					YFilter::Message::Category::Process,
					pMsg, sizeof(YFILTER_MESSAGE), false))
				{
					//	pMsg는 SendMessage 성공 후 해제될 것이다. 
					MessageThreadPool()->Alert(YFilter::Message::Category::Process);
					pMsg	= NULL;
				}
				else {
					CMemory::Free(pMsg);
					pMsg	= NULL;
				}
				//	이전 프로세스에 대한 모듈 정보도 올린다.
				//AddEarlyModule(hPID);
			}
			*/
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

	REG_CALLBACK_ARG	arg;

	arg.pArgument	= pArgument;
	arg.pArgument2	= pArgument2;
	arg.status		= STATUS_SUCCESS;
	arg.notifyClass	= (REG_NOTIFY_CLASS)(ULONG_PTR)pArgument;;



	//__log("%-32s pArgument2=%p, KeGetCurrentIrql()=%d", __FUNCTION__, pArgument2, KeGetCurrentIrql());

	if (NULL == pArgument2 ||
		KeGetCurrentIrql() > APC_LEVEL ||
		PsGetCurrentProcessId() <= (HANDLE)4 ||
		KernelMode == ExGetPreviousMode()
	)	return arg.status;

	CFltObjectReference	filter(Config()->pFilter);
	//kvnif (!filter.Reference())	return status;

	arg.PID	= PsGetCurrentProcessId();
	{	
		REG_CALLBACK_ARG	*p	= (REG_CALLBACK_ARG *)&arg;			
		switch (p->notifyClass)
		{
			case RegNtQueryValueKey:
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