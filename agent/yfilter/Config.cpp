#include "yfilter.h"

static	CONFIG	g_config;

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

		KeInitializeSpinLock(&pConfig->client.command.lock);
		KeInitializeSpinLock(&pConfig->client.event.lock);
		GetSystemRootPath(&pConfig->systemRootPath);
		__log("%-30s %wZ", "systemRootPath", &pConfig->systemRootPath);
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
		if (pConfig->pZwQueryInformationProcess	&& pConfig->pZwTerminateProcess		&&
			pConfig->pObRegisterCallbacks		&& pConfig->pObUnRegisterCallbacks)
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
	}
}