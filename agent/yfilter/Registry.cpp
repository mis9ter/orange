#include "yfilter.h"

static	LARGE_INTEGER	g_nCookie;

/*
typedef struct _REG_DELETE_KEY_INFORMATION {
	PVOID Object;
	PVOID CallContext;
	PVOID ObjectContext;
	PVOID Reserved;
} REG_DELETE_KEY_INFORMATION, *PREG_DELETE_KEY_INFORMATION, REG_FLUSH_KEY_INFORMATION, *PREG_FLUSH_KEY_INFORMATION;
typedef struct _REG_SET_VALUE_KEY_INFORMATION {
	PVOID           Object;
	PUNICODE_STRING ValueName;
	ULONG           TitleIndex;
	ULONG           Type;
	PVOID           Data;
	ULONG           DataSize;
	PVOID           CallContext;
	PVOID           ObjectContext;
	PVOID           Reserved;
} REG_SET_VALUE_KEY_INFORMATION, *PREG_SET_VALUE_KEY_INFORMATION;
typedef struct _REG_DELETE_VALUE_KEY_INFORMATION {
	PVOID           Object;
	PUNICODE_STRING ValueName;
	PVOID           CallContext;
	PVOID           ObjectContext;
	PVOID           Reserved;
} REG_DELETE_VALUE_KEY_INFORMATION, *PREG_DELETE_VALUE_KEY_INFORMATION;
typedef struct _REG_RENAME_KEY_INFORMATION {
	PVOID           Object;
	PUNICODE_STRING NewName;
	PVOID           CallContext;
	PVOID           ObjectContext;
	PVOID           Reserved;
} REG_RENAME_KEY_INFORMATION, *PREG_RENAME_KEY_INFORMATION;
*/
BOOLEAN	NTAPI	GetRegistryPath
(
	IN	PVOID				pObject,
	OUT	PUNICODE_STRING *	pObjectName
)
{
	bool			bRet = false;
	NTSTATUS		status = STATUS_SUCCESS;
	ULONG			nSize = AGENT_PATH_SIZE * sizeof(WCHAR);
	PUNICODE_STRING	pName = NULL;
	__try
	{
		if (NULL == pObject || NULL == pObjectName)	__leave;
		pName = (PUNICODE_STRING)CMemory::Allocate(PagedPool, nSize + sizeof(UNICODE_STRING), 'GRGP'); 		
		if (NULL == pName) {
			__log("%s memory allocation failed. %d bytes", __FUNCTION__, nSize + sizeof(UNICODE_STRING));
			__leave;
		}
		RtlZeroMemory(pName, nSize);
		pName->Length = (USHORT)nSize - sizeof(WCHAR);
		pName->MaximumLength = (USHORT)nSize;
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
		//status	= STATUS_UNSUCCESSFUL;
		if (NT_FAILED(status))
		{
			__log("ObQueryNameString() failed. status=%08x", status);
			__leave;
		}
		bRet = true;
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
/*
static	bool		IsAllowedOperation(PVOID pObject)
{
	bool			bRet = true;
	PUNICODE_STRING	pValue = NULL;

	if (GetRegistryPath(pObject, &pValue)) {
		if (IsObject(YFilter::Object::Protect, YFilter::Object::Type::Registry, pValue))
		{
			CWSTRBuffer	proc;
			if (NT_SUCCESS(GetProcessImagePathByProcessId(PsGetCurrentProcessId(), (PWSTR)proc, proc.CbSize(), NULL)))
			{
				bool	bAllowedProcess		= IsObject(YFilter::Object::Mode::Allow, 
												YFilter::Object::Type::Process, (PWSTR)proc);
				bool	bIsInProtectedPath	= IsObject(YFilter::Object::Mode::Protect,
												YFilter::Object::Type::File, (PWSTR)proc);
				if (bAllowedProcess || bIsInProtectedPath)
				{
					//	해당 프로세스가 허용 목록에 존재하거나 
					//	해당 프로세스의 실행 경로가 보호 대상에 존재하면 통과
				}
				else
				{
					bRet = false;
#ifndef _WIN64
					__log("denied[%d %d]: %wZ %ws(%d)", bAllowedProcess, bIsInProtectedPath,
						pName, (PWSTR)proc, (DWORD)(PsGetCurrentProcessId()));
#endif
				}
			}
		}
	}
	if (pValue)
	{
		CMemory::Free(pValue);
	}
	return bRet;
}
*/

void		RegistryLog(PCSTR pType, HANDLE PID, PVOID pObject)
{
	PUNICODE_STRING	pValue = NULL;
	if (GetRegistryPath(pObject, &pValue)) {
		__log("%-32s %6d %wZ", pType, PID, pValue);
	}
	else {
		__log("%-32s %6d GetRegistryPath() failed.", pType, PID);
	}
	if (pValue)
	{
		CMemory::Free(pValue);
	}
}


NTSTATUS	RegistryCallback
(
	IN	PVOID	pCallbackContext,
	IN	PVOID	pArgument,
	IN	PVOID	pArgument2
)
/*
NTSTATUS	RegistryCallback
(
	IN	PVOID				pCallbackContext,
	IN REG_NOTIFY_CLASS     notifyClass,
	IN	PVOID				pArgument2
)
*/
{
	NTSTATUS			status = STATUS_SUCCESS;

	UNREFERENCED_PARAMETER(pCallbackContext);
	UNREFERENCED_PARAMETER(pArgument2);

	//__log("%-32s pArgument2=%p, KeGetCurrentIrql()=%d", __FUNCTION__, pArgument2, KeGetCurrentIrql());

	if (NULL == pArgument2 ||
		KeGetCurrentIrql() > APC_LEVEL ||
		PsGetCurrentProcessId() <= (HANDLE)4 ||
		KernelMode == ExGetPreviousMode()
	)	return status;

	CFltObjectReference	filter(Config()->pFilter);
	//kvnif (!filter.Reference())	return status;

	REG_NOTIFY_CLASS	notifyClass = (REG_NOTIFY_CLASS)(ULONG_PTR)pArgument;;
	typedef struct _BASE_REG_KEY_INFO
	{
		PVOID		pObject;
		PVOID		reserved;

	} BASE_REG_KEY_INFO, *PBASE_REG_KEY_INFO;

	HANDLE	PID	= PsGetCurrentProcessId();

	switch (notifyClass)
	{
	case RegNtSetValueKey:
		//RegistryLog("RegNtSetValueKey", PID, ((PBASE_REG_KEY_INFO)pArgument2)->pObject);
		break;
	case RegNtDeleteValueKey:
		RegistryLog("RegNtDeleteValueKey", PID, ((PBASE_REG_KEY_INFO)pArgument2)->pObject);
		break;

	case RegNtRenameKey:
		RegistryLog("RegNtRenameKey", PID, ((PBASE_REG_KEY_INFO)pArgument2)->pObject);
		break;

	case RegNtPreCreateKey:

		break;
	case RegNtDeleteKey:
		RegistryLog("RegNtDeleteKey", PID, ((PBASE_REG_KEY_INFO)pArgument2)->pObject);
	break;
	case RegNtPreSetKeySecurity:

		break;
	}
	return status;
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