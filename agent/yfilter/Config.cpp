#include "yfilter.h"

static	CONFIG	g_config;

#define	REG_MACHINEGUID_KEY		L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\Cryptography"
#define REG_MACHINEGUID_VALUE	L"MachineGuid"
#define REG_BOOTID_KEY			L"\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management\\PrefetchParameters"
#define REG_BOOTID_VALUE		L"BootId"

CONFIG *	Config()
{
	return &g_config;
}
PVOID	GetProcAddress(IN PWSTR pName)
{
	PVOID			p	= NULL;
	UNICODE_STRING	name;
	if( NULL == pName )	return NULL;
	RtlInitUnicodeString(&name, pName);
	p	= MmGetSystemRoutineAddress(&name);
	if( NULL == p )		__log("%-20s %ws=%p", __FUNCTION__, pName, p);
	return p;
}
#define	__set_proc_address(ptr,name)	#ptr->p#name = (FN_#name)GetProcAddress(L#name)

bool	QueryRegistryDW(PCWSTR pRegKey, PCWSTR pRegName, DWORD * pRegValue)
{
	bool		bRet = false;
	PKEY_VALUE_PARTIAL_INFORMATION	pValue = NULL;
	NTSTATUS	status;

	UNICODE_STRING	regKey;
	CWSTR			regKeyStr(pRegKey);

	RtlInitUnicodeString(&regKey, regKeyStr);
	__log("%s %wZ", __FUNCTION__, &regKey);
	if (NT_SUCCESS(status = CDriverRegistry::QueryValue(&regKey, pRegName, &pValue)))
	{
		if (REG_DWORD == pValue->Type)
		{
			if (pValue->Data && sizeof(DWORD) == pValue->DataLength)
			{
				*pRegValue	= *pValue->Data;
				bRet	= true;
			}
		}
		CDriverRegistry::FreeValue(pValue);
	}
	else {
		__log("%s CDriverRegistry::QueryValue() failed.", __FUNCTION__);
	}
	return bRet;
}
bool	QueryRegistryString(PCWSTR pRegKey, PCWSTR pRegName, PWSTR pRegValue, size_t nValueSize)
{
	bool		bRet = false;
	PKEY_VALUE_PARTIAL_INFORMATION	pValue = NULL;
	NTSTATUS	status;

	UNICODE_STRING	regKey;
	CWSTR			regKeyStr(pRegKey);

	RtlInitUnicodeString(&regKey, regKeyStr);
	__log("%s %wZ", __FUNCTION__, &regKey);
	if (NT_SUCCESS(status = CDriverRegistry::QueryValue(&regKey, pRegName, &pValue)))
	{
		if (REG_SZ == pValue->Type)
		{
			if (pValue->Data && pValue->DataLength )
			{
				RtlStringCbCopyNW(pRegValue, nValueSize, (PCWSTR)pValue->Data, pValue->DataLength);
				bRet	= true;
			}			
		}
		CDriverRegistry::FreeValue(pValue);
	}
	else {
		__log("%s CDriverRegistry::QueryValue() failed.", __FUNCTION__);
	}
	return bRet;
}
bool	CreateConfig(IN PUNICODE_STRING pRegistryPath)
{	
	if(UseLog())	__log(__FUNCTION__);
	CONFIG *	pConfig	= &g_config;
	CWSTRBuffer	buf;
   	  
	__try
	{
		RtlZeroMemory(pConfig, sizeof(CONFIG));
		pConfig->version.dwOSVersionInfoSize	= sizeof(pConfig->version);
		RtlGetVersion((RTL_OSVERSIONINFOW *)&pConfig->version);
		pConfig->dwCPU			= KeQueryActiveProcessorCount(NULL);
		pConfig->flag.nRun		= 0;
		pConfig->flag.nProcess	= 0;

		KeInitializeSpinLock(&pConfig->lock);
		KeInitializeSpinLock(&pConfig->client.command.lock);
		KeInitializeSpinLock(&pConfig->client.event.lock);
		GetSystemRootPath(&pConfig->systemRootPath);
		QueryRegistryString(REG_MACHINEGUID_KEY, REG_MACHINEGUID_VALUE, buf, buf.CbSize());
		QueryRegistryDW(REG_BOOTID_KEY, REG_BOOTID_VALUE, &pConfig->bootId);

		__log("%-32s %wZ",	"systemRootPath", &pConfig->systemRootPath);
		__log("%-32s %ws",	"MachineGuid", (PCWSTR)buf);
		__log("%-32s %d",	"BootId", pConfig->bootId);
		__log("%-32s %d",	"CPU", pConfig->dwCPU);

		UNICODE_STRING	machineGuid;
		RtlInitUnicodeString(&machineGuid, buf);
		CMemory::AllocateUnicodeString(NonPagedPoolNx, &pConfig->machineGuid, &machineGuid);
		if (CMemory::AllocateUnicodeString(NonPagedPoolNx, &pConfig->registry, pRegistryPath))
		{
			CWSTR	reg(&pConfig->registry);
			if( reg.CbSize() )
			{
				const WCHAR *	p = wcsrchr(reg, L'\\');
				if (p)
				{
					if( false == CMemory::AllocateUnicodeString(NonPagedPoolNx, &pConfig->name, p+1) )
					{
						reg.Clear();
						__leave;
					}
				}
			}
			else
			{
				__leave;
			}
		}
		else
		{
			__log("%s allocation(1) failed.", __FUNCTION__);
			__leave;
		}
		if( (PCWSTR)buf )
		{
			RtlStringCbPrintfW(buf, buf.CbSize(), L"\\Device\\%wZ", &pConfig->name);
			if( false == CMemory::AllocateUnicodeString(NonPagedPoolNx, &pConfig->deviceName, buf) )
				__leave;
			RtlStringCbPrintfW(buf, buf.CbSize(), L"\\DosDevices\\%wZ", &pConfig->name);
			if( false == CMemory::AllocateUnicodeString(NonPagedPoolNx, &pConfig->dosDeviceName, buf) )
				__leave;
		}
		else
		{
			__log("%s allocation(2) failed.", __FUNCTION__);
			__leave;
		}
		if (CDriverCommon::GetDriverValueFromRegistry(pRegistryPath, L"ImagePath", buf, buf.CbSize()))
		{
			if( false == CMemory::AllocateUnicodeString(NonPagedPoolNx, &pConfig->imagePath, buf) )
				__leave;
		}
		else
		{
			__log("%s GetDriverValueFromRegistry() failed.", __FUNCTION__);
			__leave;
		}
		CreateObjectTable(pRegistryPath);
		pConfig->pZwQueryInformationProcess = (FN_ZwQueryInformationProcess)GetProcAddress(L"ZwQueryInformationProcess");
		pConfig->pZwTerminateProcess		= (FN_ZwTerminateProcess)GetProcAddress(L"ZwTerminateProcess");
		pConfig->pObRegisterCallbacks		= (FN_ObRegisterCallbacks)GetProcAddress(L"ObRegisterCallbacks");
		pConfig->pObUnRegisterCallbacks		= (FN_ObUnRegisterCallbacks)GetProcAddress(L"ObUnRegisterCallbacks");
		pConfig->pNtQueryInformationThread	= (FN_NtQueryInformationThread)GetProcAddress(L"NtQueryInformationThread");
		pConfig->pPsSetCreateThreadNotifyRoutineEx	= (FN_PsSetCreateThreadNotifyRoutineEx)GetProcAddress(L"PsSetCreateThreadNotifyRoutineEx");
		pConfig->pZwQueryInformationThread	= (FN_ZwQueryInformationThread)GetProcAddress(L"ZwQueryInformationThread");
		pConfig->pLoadLibraryA				= (FN_LoadLibraryA)GetProcAddress(L"LoadLibraryA");
		pConfig->pLoadLibraryW				= (FN_LoadLibraryW)GetProcAddress(L"LoadLibraryW");
		pConfig->pZwQuerySystemInformation	= (FN_ZwQuerySystemInformation)GetProcAddress(L"ZwQuerySystemInformation");
		pConfig->pSeLocateProcessImageName	= (FN_SeLocateProcessImageName)GetProcAddress(L"SeLocateProcessImageName");

		/*
		__log("%s pZwQueryInformationProcess        %p", __FUNCTION__, pConfig->pZwQueryInformationProcess);
		__log("%s pZwTerminateProcess               %p", __FUNCTION__, pConfig->pZwTerminateProcess);
		__log("%s pObRegisterCallbacks              %p", __FUNCTION__, pConfig->pObRegisterCallbacks);
		__log("%s pObUnRegisterCallbacks            %p", __FUNCTION__, pConfig->pObUnRegisterCallbacks);
		__log("%s pNtQueryInformationThread         %p", __FUNCTION__, pConfig->pNtQueryInformationThread);
		__log("%s pPsSetCreateThreadNotifyRoutineEx %p", __FUNCTION__, pConfig->pPsSetCreateThreadNotifyRoutineEx);
		__log("%s pZwQueryInformationThread         %p", __FUNCTION__, pConfig->pZwQueryInformationThread);
		__log("%s pZwQuerySystemInformation         %p", __FUNCTION__, pConfig->pZwQuerySystemInformation);
		__log("%s pLoadLibraryA                     %p", __FUNCTION__, pConfig->pLoadLibraryA);
		__log("%s pLoadLibraryW                     %p", __FUNCTION__, pConfig->pLoadLibraryW);
		*/
		if (pConfig->pZwQueryInformationProcess	&& pConfig->pZwTerminateProcess		&&
			pConfig->pObRegisterCallbacks		&& pConfig->pObUnRegisterCallbacks	&& 
			pConfig->pNtQueryInformationThread	)
		{
			pConfig->bInitialized = true;
		}
		else
			__leave;
	}
	__finally
	{
		buf.Clear();
	}
	return pConfig->bInitialized;
}
void	DestroyConfig()
{
	__log(__FUNCTION__);
	CONFIG *	pConfig	= Config();

	if( pConfig )
	{
		DestroyObjectTable(&pConfig->registry);
		CMemory::FreeUnicodeString(&pConfig->registry);
		CMemory::FreeUnicodeString(&pConfig->name);
		CMemory::FreeUnicodeString(&pConfig->dosDeviceName);
		CMemory::FreeUnicodeString(&pConfig->deviceName);
		CMemory::FreeUnicodeString(&pConfig->imagePath);
		CMemory::FreeUnicodeString(&pConfig->systemRootPath);
		CMemory::FreeUnicodeString(&pConfig->machineGuid);
	}
}