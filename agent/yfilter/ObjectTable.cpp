#include "yfilter.h"

bool		IsObject(
	YFilter::Object::Mode	mode,
	YFilter::Object::Type	type,
	PUNICODE_STRING			pValue
)
{
	POBJECT_TABLE	pTable = ObjectTable(mode, type);
	return pTable ? OtIsExisting(pTable, pValue) : false;
}
bool		IsObject(
	YFilter::Object::Mode	mode,
	YFilter::Object::Type	type, 
	PCWSTR			pValue
)
{
	POBJECT_TABLE	pTable = ObjectTable(mode, type);
	return pTable? OtIsExisting(pTable, pValue) : false;
}
bool		AddObject(
	YFilter::Object::Mode	mode,
	YFilter::Object::Type	type,
	PWSTR			pValue
)
{
	__log("%s", __FUNCTION__);
	POBJECT_TABLE	pTable = ObjectTable(mode, type);
	return pTable? OtAdd(pTable, pValue) : false;
}
DWORD		ListObject(
	YFilter::Object::Mode	mode,
	YFilter::Object::Type	type,
	PVOID					ptr, 
	DWORD					dwPtr, 
	PEnumObject				func
)
{
	__log("%s", __FUNCTION__);
	POBJECT_TABLE	pTable = ObjectTable(mode, type);
	return pTable ? OtList(pTable, ptr, dwPtr, func) : 0;
}
bool		RemoveObject(
	YFilter::Object::Mode	mode,
	YFilter::Object::Type	type,
	PWSTR			pValue
)
{
	__log("%s	%ws", __FUNCTION__, pValue? pValue : L"(null)");
	if( NULL == pValue )	return false;
	POBJECT_TABLE	pTable = ObjectTable(mode, type);
	return pTable ? OtRemove(pTable, pValue) : false;
}
bool		RemoveAllObject(
	YFilter::Object::Mode	mode,
	YFilter::Object::Type	type
)
{
	__log("%s", __FUNCTION__);
	POBJECT_TABLE	pTable = ObjectTable(mode, type);
	return pTable ? OtRemoveAll(pTable) : false;
}
typedef struct OBJECT_TABLE_CONFIG
{
	bool					bCreated;
	struct {
		OBJECT_TABLE		objects[YFilter::Object::Type::End];
		HANDLE				hProcess;
	} protect;
	struct {
		OBJECT_TABLE		objects[YFilter::Object::Type::End];
		HANDLE						hProcess;
	} allow;
	/*
		[TODO]
		아래 부분은 마음에는 들지 않지만 현재 별다른 방법이 없으므로 
		우선은 이렇게 처리한다. 

		kill	
		해당 목록(경로, 실행파일명)의 프로세스가 접근 시 kill 시킴.
		notopen
		해당 목록(경로, 실행파일명)의 프로세스가 접근 시 거부함.

		이 목록에 해당되는 녀석들은 '전체 경로' 로도 검색하고 '프로세스명' 만으로도 검색 대상. 
	* */
	struct {
		OBJECT_TABLE		objects[3];
	} etc;
	
} OBJECT_TABLE_CONFIG, OTC, *POTC, *POBJECT_TABLE_CONFIG;

static	OBJECT_TABLE_CONFIG	g_objectTable;

POBJECT_TABLE	ObjectTable(IN YFilter::Object::Mode mode, IN YFilter::Object::Type type)
{
	POBJECT_TABLE			pTable	= NULL;
	switch (mode)
	{
		case YFilter::Object::Mode::Protect:
			switch (type)
			{
			case YFilter::Object::Type::File:
			case YFilter::Object::Type::Process:
			case YFilter::Object::Type::Registry:
			case YFilter::Object::Type::Memory:
				pTable	= &g_objectTable.protect.objects[mode];
				break;
			}
			break;
		case YFilter::Object::Mode::Allow:
			switch (type)
			{
				case YFilter::Object::Type::File:
				case YFilter::Object::Type::Process:
				case YFilter::Object::Type::Registry:
				case YFilter::Object::Type::Memory:
					return &g_objectTable.allow.objects[mode];
			}
			break;
		case YFilter::Object::Mode::White:
			pTable	= &g_objectTable.etc.objects[0];
			break;
		case YFilter::Object::Mode::Gray:
			pTable	= &g_objectTable.etc.objects[1];
			break;
		case YFilter::Object::Mode::Black:
			pTable	= &g_objectTable.etc.objects[2];
			break;
		default:
			break;

	}
	if( NULL == pTable )
		__log("%s pTable is null. mode=%d, type=%d", __FUNCTION__, mode, type);
	return pTable;
}
bool			OtIsPossible()
{
	if (KeGetCurrentIrql() != PASSIVE_LEVEL)	return false;
	return true;
}
void			CreateObjectTable(PUNICODE_STRING pRegistry)
{
	if( UseLog() )	__log("%s", __FUNCTION__);
	WCHAR	*pProtectNames[] = {
		PROTECT_FILE_NAME, PROTECT_PROCESS_NAME, PROTECT_REGISTRY_NAME, PROTECT_MEMORY_NAME,
	};
	WCHAR	*pAllowNames[] = {
		ALLOW_FILE_NAME, ALLOW_PROCESS_NAME, ALLOW_REGISTRY_NAME, ALLOW_MEMORY_NAME,
	};
	WCHAR	*pEtcNames[] = {
		WHITE_PROCESS_NAME, GRAY_PROCESS_NAME, BLACK_PROCESS_NAME,
	};
	__try
	{
		RtlZeroMemory(&g_objectTable, sizeof(g_objectTable));
		g_objectTable.bCreated	= true;
		for (auto i = 0; i < YFilter::Object::Type::End; i++)
			OtInitialize(pRegistry, YFilter::Object::Mode::Protect, 
				(YFilter::Object::Type)i, pProtectNames[i], 
				&g_objectTable.protect.objects[i]);
		for (auto i = 0; i < YFilter::Object::Type::End; i++)
			OtInitialize(pRegistry, YFilter::Object::Mode::Allow, 
				(YFilter::Object::Type)i, pAllowNames[i], 
				&g_objectTable.allow.objects[i]);
		for( auto i = 0 ; i < _countof(g_objectTable.etc.objects) ; i++)
			OtInitialize(pRegistry, (YFilter::Object::Mode)(YFilter::Object::Mode::White+i), 
					YFilter::Object::Type::Process, pEtcNames[i], 
					&g_objectTable.etc.objects[i]);

		OtAdd(ObjectTable(YFilter::Object::Mode::Black, YFilter::Object::Type::Process), L"TaskMgr.exe");
		OtAdd(ObjectTable(YFilter::Object::Mode::Black, YFilter::Object::Type::Process), L"ProcessHacker.exe");
		OtAdd(ObjectTable(YFilter::Object::Mode::Black, YFilter::Object::Type::Process), L"ProcessExplorer.exe");
		OtAdd(ObjectTable(YFilter::Object::Mode::White, YFilter::Object::Type::Process), L"ASDSvc.exe");		//	V3
		OtAdd(ObjectTable(YFilter::Object::Mode::Protect, YFilter::Object::Type::Registry), 
			L"\\REGISTRY\\MACHINE\\SOFTWARE\\Geni\\Insights");
	}
	__finally
	{
		if(UseLog())	__log("%-20s %d bytes", "Object Table", sizeof(g_objectTable));
	}
}
void			DestroyObjectTable(IN PUNICODE_STRING pRegistry)
{
	if (NULL == pRegistry)
	{
		__log("%s  pRegistry=%p", pRegistry);
	}
	__try
	{
		if( g_objectTable.bCreated )
		{
			for (auto i = 0; i < YFilter::Object::Type::End; i++)
				OtDestroy(pRegistry, &g_objectTable.protect.objects[i]);
			for (auto i = 0; i < YFilter::Object::Type::End; i++)
				OtDestroy(pRegistry, &g_objectTable.allow.objects[i]);
			OtDestroy(pRegistry, &g_objectTable.etc.objects[0]);
			OtDestroy(pRegistry, &g_objectTable.etc.objects[1]);
			OtDestroy(pRegistry, &g_objectTable.etc.objects[2]);
			g_objectTable.bCreated	= false;
		}
	}
	__finally
	{

	}
}
inline void		XORBuffer(char * pBuffer, SIZE_T bufferLength)
{
	static const UCHAR TARGET_COOKIE = 0xA7;
	for (SIZE_T i = 0; i < bufferLength; ++i)
	{
		if (pBuffer[i] != 0)
		{
			pBuffer[i] ^= TARGET_COOKIE;
		}
	}
}
bool			OtLoad(PUNICODE_STRING pRegistry, PCWSTR pName, POBJECT_TABLE pTable)
{
	bool		bRet = false;
	PKEY_VALUE_PARTIAL_INFORMATION	pValue = NULL;
	size_t		nSliceSize = TARGET_OBJECT_SIZE * sizeof(WCHAR);
	NTSTATUS	status;

	if (NULL == pRegistry || NULL == pName || NULL == pTable)
	{
		__log("%s  pRegistry=%p, pName=%p, pTable=%p", __FUNCTION__, pRegistry, pName, pTable);
		return false;
	}
	UNREFERENCED_PARAMETER(pTable);
	if (NT_SUCCESS(status = CDriverRegistry::QueryValue(pRegistry, pName, &pValue)))
	{
		if (REG_BINARY == pValue->Type)
		{
			if (pValue->Data && pValue->DataLength && 0 == (pValue->DataLength % nSliceSize))
			{
				size_t	nCount = pValue->DataLength / nSliceSize;
				PWSTR	pStr = (PWSTR)CMemory::AllocateString(PagedPool, nSliceSize, 'CUP2');
				if (pStr)
				{
					XORBuffer((char *)pValue->Data, pValue->DataLength);
					for (unsigned i = 0; i < nCount && (i * nSliceSize) < pValue->DataLength; i++)
					{
						RtlStringCbCopyW(pStr, nSliceSize, (PWSTR)(pValue->Data + i * nSliceSize));
						OtAdd(pTable, pStr);
					}
					CMemory::FreeString(pStr);
				}
			}
		}
		CDriverRegistry::FreeValue(pValue);
	}
	return bRet;
}
void			OtInitialize(PUNICODE_STRING pRegistry, 
	IN YFilter::Object::Mode mode, IN YFilter::Object::Type type, IN PWSTR pName, IN POBJECT_TABLE pTable)
{
	if (NULL == pRegistry || NULL == pName || NULL == pTable)
	{
		__log("%s pRegistry=%p, pName=%p, pTable=%p", __FUNCTION__, pRegistry, pName, pTable);
		return;
	}
	if( UseLog() )	__log("%s mode=%ws(%d), type=%ws(%d), pName=%ws", 
		__FUNCTION__, 
		YFilter::GetModeName(mode), mode, YFilter::GetTypeName(type), type, pName);
	__try
	{
		pTable->mode	= mode;
		pTable->type	= type;
		RtlStringCbCopyW(pTable->szName, sizeof(pTable->szName), pName);
		ExInitializeFastMutex(&pTable->lock);
		RtlInitializeUnicodePrefix(pTable);
		OtLoad(pRegistry, pName, pTable);
		//__log("%-20s %d %d %ws", __FUNCTION__, p->mode, p->type, p->szName);
	}
	__finally
	{

	}
}
void			OtDestroy(IN PUNICODE_STRING pRegistry, IN POBJECT_TABLE pTable)
{
	if (NULL == pRegistry || NULL == pTable)
	{
		__log("%s  pRegistry=%p, pTable=%p", __FUNCTION__, pRegistry, pTable);
		return;
	}
	__try
	{
		if (NULL == pRegistry || NULL == pTable )	__leave;
		//__log("%-20s %d %d %ws", __FUNCTION__, p->category, p->type, p->szName);
		OtSave(pRegistry, pTable);
		OtRemoveAll(pTable);
	}
	__finally
	{

	}
}
bool			OtAdd(IN POBJECT_TABLE pTable, IN PCWSTR pStr)
{
	if (NULL == pTable || NULL == pStr)
	{
		__log("%s  pTable=%p, pStr=%p", __FUNCTION__, pTable, pStr);
		return false;
	}
	if (L'\0' == *pStr || false == OtIsPossible())
	{
		__log("%s pStr=%ws, OtIsPossible()=%d", __FUNCTION__, pStr, OtIsPossible());
		return false;
	}
	bool	bRet = false;
	POBJECT_ENTRY	pEntry = NULL;
	__try
	{
		if (OtIsExisting(pTable, pStr))
		{
			if(UseLog())	__log("%s %ws %ws is already existing.", __FUNCTION__, pTable->szName, pStr);
			__leave;
		}
		pEntry = (POBJECT_ENTRY)CMemory::Allocate(NonPagedPoolNx, sizeof(OBJECT_ENTRY), 'CUP3');
		if (pEntry)
		{
			RtlZeroMemory(pEntry, sizeof(OBJECT_ENTRY));
			if (CMemory::AllocateUnicodeString(NonPagedPoolNx, &pEntry->data, pStr))
			{
				CFastMutexLocker::Lock(&pTable->lock);
				if (RtlInsertUnicodePrefix(pTable, &pEntry->data, (PUNICODE_PREFIX_TABLE_ENTRY)pEntry))
				{
					pTable->wCount++;
					bRet	= true;
					__log("%-10ws %-10ws %ws: %ws",
						YFilter::GetModeName(pTable->mode), 
						YFilter::GetTypeName(pTable->type), pTable->szName, pStr);
				}
				CFastMutexLocker::Unlock(&pTable->lock);
			}
		}
		else
		{
			__log("  pEntry is null. memory allocation failed. size=%d", sizeof(OBJECT_ENTRY));
		}
	}
	__finally
	{
		if (false == bRet)
		{
			if(UseLog())	__log("%s failed. length=%d", __FUNCTION__, (int)wcslen(pStr));
			if (pEntry)
			{
				if (pEntry->data.Buffer)
				{
					CMemory::FreeString(pEntry->data.Buffer);
				}
				CMemory::Free(pEntry);
			}
		}
	}
	return bRet;
}
bool			OtAdd(IN POBJECT_TABLE pTable, IN PWSTR pStr)
{
	if (NULL == pStr || L'\0' == *pStr || NULL == pTable || false == OtIsPossible())
	{
		return false;
	}
	size_t	nLength = wcslen(pStr);
	if (L'\\' == pStr[nLength - 1])
	{
		//	들어가는 경로 끝에 \가 있으면 검색이 되지 않는다.
		//	https://m.blog.naver.com/PostView.nhn?blogId=gloryo&logNo=110117264622&proxyReferer=https%3A%2F%2Fwww.google.com%2F
		pStr[nLength - 1] = L'\0', nLength--;
	}
	return OtAdd(pTable, (PCWSTR)pStr);
}
bool			OtRemove(IN POBJECT_TABLE pTable, IN PCWSTR pStr)
{
	if (NULL == pStr || false == OtIsPossible())	return false;
	UNREFERENCED_PARAMETER(pTable);
	UNREFERENCED_PARAMETER(pStr);
	bool	bRet = false;
	UNICODE_STRING		ustr = { 0, };

	RtlInitUnicodeString(&ustr, pStr);
	__try
	{
		CFastMutexLocker::Lock(&pTable->lock);
		auto	pEntry = (POBJECT_ENTRY)RtlFindUnicodePrefix(pTable, &ustr, 0);
		if (pEntry)
		{
			RtlRemoveUnicodePrefix(pTable, pEntry);
			__log("[%d] REMOVE %ws: %ws", pTable->type, pTable->szName, pStr);
			if (pEntry->data.Buffer)
			{
				CMemory::FreeUnicodeString(&pEntry->data);
			}
			CMemory::Free(pEntry);
			if (pTable->wCount)	pTable->wCount--;
			__dlog("  removed.");
			bRet = true;
		} 
		else
		{
			__dlog("  not exist.");
		}
		CFastMutexLocker::Unlock(&pTable->lock);
	}
	__finally
	{

	}
	return bRet;
}
bool			OtRemove(IN POBJECT_TABLE pTable, IN PUNICODE_STRING p)
{
	UNREFERENCED_PARAMETER(pTable);
	UNREFERENCED_PARAMETER(p);
	bool	bRet = false;
	__try
	{
		bRet = true;
	}
	__finally
	{

	}
	return bRet;
}
bool			OtRemoveAll(IN POBJECT_TABLE pTable)
{
	if( UseLog() )	__log("%s", __FUNCTION__);
	if (false == OtIsPossible())	return false;
	bool	bRet = false;
	__try
	{
		CFastMutexLocker::Lock(&pTable->lock);
		for (auto pEntry = (POBJECT_ENTRY)RtlNextUnicodePrefix(pTable, TRUE);
			pEntry != NULL; pEntry = (POBJECT_ENTRY)RtlNextUnicodePrefix(pTable, FALSE))
		{
			if (pEntry)
			{
				if(UseLog())	__log("  REMOVE [%d][%d][%ws] %wZ", pTable->mode, pTable->type, pTable->szName, &pEntry->data);
				RtlRemoveUnicodePrefix(pTable, pEntry);
				if (pEntry->data.Buffer)
				{
					CMemory::FreeUnicodeString(&pEntry->data);
				}
				CMemory::Free(pEntry);
			}
		}
		bRet = true;
	}
	__finally
	{
		CFastMutexLocker::Unlock(&pTable->lock);
	}
	return bRet;
}
bool			OtIsExisting(IN POBJECT_TABLE pTable, IN PCWSTR p)
{
	bool			bRet = false;
	UNICODE_STRING	str;
	if (NULL == p)
	{
		__log("%s ERROR - p is null.", __FUNCTION__);
		return false;
	}
	RtlInitUnicodeString(&str, p);
	bRet	= OtIsExisting(pTable, &str);
	return bRet;
}
ULONG			OtList(IN POBJECT_TABLE pTable)
{
	ULONG	nCount	= 0;
	if (false == OtIsPossible() || NULL == pTable) {
		__log("%s OtIsPossible()=%d, pTable=%p", __FUNCTION__, OtIsPossible(), pTable);
		return 0;
	}
	POBJECT_ENTRY	pEntry	= NULL;
	__try
	{
		CFastMutexLocker::Lock(&pTable->lock);
		/*
			특별한 이유가 없는데도 RtlFindUnicodePrefix 에서 문제가 발생한다면
			가슴에 손은 얹고 pTable 이 초기화 되지 않았는지 반성해보도록.
		* */
		for (pEntry = (POBJECT_ENTRY)RtlNextUnicodePrefix(pTable, TRUE);
			pEntry != NULL; pEntry = (POBJECT_ENTRY)RtlNextUnicodePrefix(pTable, FALSE), nCount++)
		{
			if (pEntry)
			{
				//__dlog("  %d", nCount);
			}
		}
		if( 0 == nCount )
			__dlog("%s no count", __FUNCTION__);
	}
	__finally
	{
		CFastMutexLocker::Unlock(&pTable->lock);
	}
	return nCount;
}
bool			OtIsExisting(IN POBJECT_TABLE pTable, IN PUNICODE_STRING p)
{
	if( false == OtIsPossible() || NULL == pTable || NULL == p) {
		__log("%s OtIsPossible()=%d, pTable=%p, p=%p", __FUNCTION__, OtIsPossible(), pTable, p);
		return false;
	}
	bool	bRet = false;
	__try
	{
		CFastMutexLocker::Lock(&pTable->lock);
		/*
			특별한 이유가 없는데도 RtlFindUnicodePrefix 에서 문제가 발생한다면
			가슴에 손은 얹고 pTable 이 초기화 되지 않았는지 반성해보도록.
		* */
		POBJECT_ENTRY	pEntry = (POBJECT_ENTRY)RtlFindUnicodePrefix(pTable, p, 0);
		bRet = pEntry? true : false;
	}
	__finally
	{
		CFastMutexLocker::Unlock(&pTable->lock);
	}
	return bRet;
}
bool			OtSave(IN PUNICODE_STRING pRegistry, IN POBJECT_TABLE pTable)
{
	if( UseLog())	__log("%s", __FUNCTION__);
	UNREFERENCED_PARAMETER(pTable);
	if (false == OtIsPossible())
	{
		__log("  OtIsPossible() is false.");
		return false;
	}
	if( NULL == pRegistry || NULL == pTable )
	{
		__log("  error: pRegistry(%p) or pTable(%p) is null.", pRegistry, pTable);
		return false;
	}
	if (0 == pTable->wCount)
	{
		CDriverRegistry::DeleteValue(pRegistry, pTable->szName,	(wcslen(pTable->szName) + 1) * sizeof(WCHAR));
		return true;
	}
	bool		bRet = false;
	size_t		nSliceSize	= TARGET_OBJECT_SIZE * sizeof(WCHAR);
	size_t		nTotalSize	= nSliceSize * pTable->wCount;
	LPVOID		pData		= NULL;
	NTSTATUS	status;
	__try
	{
		pData = CMemory::Allocate(PagedPool, nTotalSize, 'CUP1');
		if (NULL == pData)
		{
			__log("  pData is null.");
			__leave;
		}
		RtlZeroMemory(pData, nTotalSize);
		CFastMutexLocker::Lock(&pTable->lock);
		{
			int		i = 0;
			for (auto pEntry = (POBJECT_ENTRY)RtlNextUnicodePrefix(pTable, TRUE);
				pEntry && i < pTable->wCount;
				pEntry = (POBJECT_ENTRY)RtlNextUnicodePrefix(pTable, FALSE), i++)
			{
				PWSTR	pStr = (PWSTR)((char *)pData + nSliceSize * i);
				RtlStringCbCopyUnicodeString(pStr, nSliceSize, &pEntry->data);
			}
		}
		CFastMutexLocker::Unlock(&pTable->lock);
		XORBuffer((char *)pData, nTotalSize);
		if (NT_SUCCESS(
				status = CDriverRegistry::SetValue(pRegistry, pTable->szName, 
								REG_BINARY, pData, (ULONG)nTotalSize)
			)
		)
			bRet = true;
	}
	__finally
	{
		if( pData )	CMemory::Free(pData);
	}
	return bRet;
}
bool			OtRestore(IN POBJECT_TABLE pTable)
{
	UNREFERENCED_PARAMETER(pTable);
	bool	bRet = false;
	__try
	{
		bRet = true;
	}
	__finally
	{

	}
	return bRet;
}
DWORD			OtList(IN POBJECT_TABLE pTable, LPVOID ptr, DWORD dwPtr, PEnumObject func)
{
	__log("%s", __FUNCTION__);
	if (NULL == pTable || NULL == pTable->szName[0] || false == OtIsPossible())	{
		__log("  pTable=%p, pTable->szName=%ws, OtIsPossible()=%d", 
			pTable, pTable? pTable->szName : L"", OtIsPossible());
		return 0;
	}
	DWORD	dwCount	= 0;
	__try
	{
		CFastMutexLocker::Lock(&pTable->lock);
		int		i = 0;
		for (auto pEntry = (POBJECT_ENTRY)RtlNextUnicodePrefix(pTable, TRUE);
			pEntry && i < pTable->wCount;
			pEntry = (POBJECT_ENTRY)RtlNextUnicodePrefix(pTable, FALSE), i++)
		{
			if (func)	func(pTable->mode, pTable->type, pTable->wCount, i, &pEntry->data, ptr, dwPtr);
			dwCount++;
		}
	}
	__finally
	{
		CFastMutexLocker::Unlock(&pTable->lock);
	}
	return dwCount;
}