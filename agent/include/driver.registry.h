#pragma once
#define		__cregistry_memory_tag	'geRC'

class CDriverRegistry
{
public:
	CDriverRegistry()
	{

	}
	~CDriverRegistry()
	{

	}
	static	NTSTATUS		QueryValue
	(
		IN	PUNICODE_STRING		path,
		IN	LPCWSTR				pValueName,
		OUT	PKEY_VALUE_PARTIAL_INFORMATION *ppValue
	)
	{
		UNICODE_STRING		valueName;
		if( NULL == pValueName )	return STATUS_UNSUCCESSFUL;
		RtlInitUnicodeString(&valueName, pValueName);
		return QueryValue(path, &valueName, ppValue);
	}
	static	NTSTATUS		QueryValue
	(
		IN	PUNICODE_STRING		path, 
		IN	PUNICODE_STRING		valueName, 
		OUT	PKEY_VALUE_PARTIAL_INFORMATION *ppValue
	)
	{
		NTSTATUS			status	= STATUS_SUCCESS;
		OBJECT_ATTRIBUTES	attributes;
		HANDLE				hKey		= NULL;
		PKEY_VALUE_PARTIAL_INFORMATION pKeyValue = NULL;
		ULONG				nLength = 0;

		__try
		{
			InitializeObjectAttributes(&attributes, path, OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE, NULL, NULL);
			status = ZwOpenKey(&hKey, KEY_READ, &attributes);
			if (!NT_SUCCESS(status))	__leave;

			status = ZwQueryValueKey(hKey, valueName, KeyValuePartialInformation, NULL, 0, &nLength);
			if (status != STATUS_BUFFER_TOO_SMALL && status != STATUS_BUFFER_OVERFLOW) {
				status = STATUS_INVALID_PARAMETER;
				__leave;
			}
			pKeyValue = (PKEY_VALUE_PARTIAL_INFORMATION)ExAllocatePoolWithTag(NonPagedPoolNx, nLength, __cregistry_memory_tag);
			if ( NULL == pKeyValue ) {
				status = STATUS_INSUFFICIENT_RESOURCES; __leave;
			}
			status = ZwQueryValueKey(hKey, valueName, KeyValuePartialInformation, pKeyValue, nLength, &nLength);
			if (!NT_SUCCESS(status))	__leave;
		}
		__finally
		{
			if (NT_SUCCESS(status))
			{
				*ppValue = pKeyValue;
			}
			else 
			{
				FreeValue(pKeyValue);
				*ppValue = NULL;
			}
			if( hKey )	ZwClose(hKey);
		}
		return status;
	}
	static	void			FreeValue(PKEY_VALUE_PARTIAL_INFORMATION pKeyValue)
	{
		if (pKeyValue)
			ExFreePoolWithTag(pKeyValue, __cregistry_memory_tag);
	}
	static	NTSTATUS		SetValue
	(
		IN PUNICODE_STRING	regPath, 
		IN const WCHAR*		pszValueName, 
		IN ULONG			nType, 
		IN PVOID			pValue, 
		IN ULONG			cbValue
	)
	{
		NTSTATUS			status = 0;
		OBJECT_ATTRIBUTES	attributes;
		HANDLE				hKey = NULL;
		UNICODE_STRING		valueName;

		if( NULL == pszValueName )	return STATUS_UNSUCCESSFUL;
		RtlInitUnicodeString(&valueName, pszValueName);
		do
		{
			InitializeObjectAttributes(&attributes, regPath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
			//	zw 계열과 nt 계열의 함수 차이
			//	zw 계열
			//	1. 함수로 전달된 인자들의 유효성을 검사하는 일을 한다. 
			//	2. 현재 사용 환경이 커널레벨로 인지.
			//	Zw함수를 써도 어차피 내부적으로 Nt함수를 가져오므로 커널에선 Zw를 쓰지 말고 바로 Nt를 쓰면 안되나?
			//	=> 결론은 써도 된다. 하지만 아주 위험하다는 것. 
			status = ZwOpenKey(&hKey, KEY_WRITE, &attributes);
			if (!NT_SUCCESS(status))
			{
				CUnicodeString	str(regPath, PagedPool);
				__log("%s	path=%wZ key=%ws ZwOpenKey() failed. status=%08x", 
					__FUNCTION__, regPath, (LPCWSTR)str, status);
				break;
			}
			status = ZwSetValueKey(hKey, &valueName, 0, nType, pValue, cbValue);
			if( !NT_SUCCESS(status))	break;
			ZwFlushKey(hKey);
		}
		while( false );
		if( hKey  )	ZwClose(hKey);
		return status;
	}
	static	NTSTATUS	DeleteValue
	(
		IN	PUNICODE_STRING	pRegistry,
		IN	const WCHAR*	pValueName,
		IN	size_t			nMaxValueNameSize
	)
	{
		NTSTATUS			status = STATUS_UNSUCCESSFUL;
		if (NULL == pRegistry || NULL == pValueName)
		{
			__log("%s pRegistry(%p) or pValueName(%p) is null.", pRegistry, pValueName);
			return status;
		}
		OBJECT_ATTRIBUTES	attributes;
		HANDLE				hKey	= NULL;
		CUnicodeString		valueName(pValueName, PagedPool, nMaxValueNameSize);

		do
		{
			InitializeObjectAttributes(&attributes, pRegistry, OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE, NULL, NULL);
			status = ZwOpenKey(&hKey, KEY_SET_VALUE, &attributes);
			if (NT_FAILED(status))
			{
				__log("%s ZwOpenKey() failed. status=%08x", __FUNCTION__, status);
				__log("path :%wZ", pRegistry);
				__log("value:%ws", pValueName);
				break;
			}
			status = ZwDeleteValueKey(hKey, valueName);
			if( NT_FAILED(status) && STATUS_OBJECT_NAME_NOT_FOUND != status ) {
				__log("%s ZwDeleteValueKey() failed. status=%08x", __FUNCTION__, status);
				__log("path :%wZ", pRegistry);
				__log("value:%ws", pValueName);
				break;
			}
			ZwFlushKey(hKey);
		}
		while( false );
		if( hKey )	ZwClose(hKey);
		return status;
	}
private:

};
