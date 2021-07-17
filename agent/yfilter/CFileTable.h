#pragma once
#include <stdint.h>

#define TAG_OBJECT			'tjbo'

typedef struct FILE_ENTRY
{
	PVOID				pKey;
	UID					FPUID;
	UID					FUID;
	HANDLE				PID;
	PROCUID				PUID;
	HANDLE				TID;
	UNICODE_STRING		FilePath;
	ULONG				nCount;
	ULONG64				nSize;
	DWORD64				dwTicks;
	PVOID				pContext;
} FILE_ENTRY, * PFILE_ENTRY;
typedef struct {
	HANDLE				PID;
	HANDLE				TID;
	ULONG				SID;
	PROCUID				PUID;
	UID					FPUID;
	UID					FUID;
	PUNICODE_STRING		pFilePath;
	NTSTATUS			status;
	ULONG				nSize;
	YFilter::Message::SubType	subType;
	PVOID				pContext;
}	FILE_CALLBACK_ARG, *PFILE_CALLBACK_ARG;
typedef void (*PFileTableCallback)(

	IN PFILE_ENTRY			pEntry,			//	대상 엔트리 
	IN PVOID				pContext
);

void		CreateFileMessage(
	PFILE_ENTRY			p,
	PY_FILE_MESSAGE		*pOut
);

class CFileTable
{
public:
	bool			IsInitialized()
	{
		return m_bInitialize;
	}
	void			Initialize(IN bool bSupportDIRQL = true)
	{
		__log(__FUNCTION__);
		RtlZeroMemory(this, sizeof(CProcessTable));
		m_bSupportDIRQL = bSupportDIRQL;
		m_pooltype		= bSupportDIRQL ? NonPagedPoolNx : PagedPool;
		ExInitializeFastMutex(&m_mutex);
		KeInitializeSpinLock(&m_lock);
		RtlInitializeGenericTable(&m_table, Compare, Alloc, Free, this);
		m_thread.Create(1, Thread, this);
		m_bInitialize = true;
	}
	static	UID		GetFilePUID(HANDLE PID, INT nClass, PUNICODE_STRING pFilePath) {
		CWSTRBuffer	buf;
		RtlStringCbPrintfW((PWSTR)buf, buf.CbSize(), L"%d.%d.%wZ", PID, nClass, pFilePath);
		UID	uid	= Path2CRC64((PCWSTR)buf);
		//__log("%-32s %p %ws", __func__, uid, (PCWSTR)buf);
		return uid;
	}
	static	UID		GetFileUID(PUNICODE_STRING pFilePath) {
		return Path2CRC64(pFilePath);
	}
	static	YFilter::Message::SubType	Class2SubType(INT nClass)
	{
		switch( nClass ) {
		case RegNtPostQueryValueKey:
		case RegNtPreQueryValueKey:
			return YFilter::Message::SubType::RegistryGetValue;
		case RegNtPreSetValueKey:
		case RegNtPostSetValueKey:
			return YFilter::Message::SubType::RegistrySetValue;
		case RegNtDeleteValueKey:
			return YFilter::Message::SubType::RegistryDeleteValue;

		default:
			__log("%-32s unknown class:%d", __func__, nClass);
		}
		return YFilter::Message::SubType::RegistryUnknown;
	}
	ULONG			Flush(HANDLE PID)
	{
		struct FLUSH_INFO
		{
			CFileTable	*pClass;
			HANDLE		PID;
			ULONG		nTicks;
			ULONG		nCount;
		} info;

		info.pClass		= this;
		info.PID		= PID;
		KeQueryTickCount(&info.nTicks);
		info.nCount		= 0;

		//__dlog("CFileTable::Flush	%d", Count());
		if( true )	return 0;

		Flush(&info, [](PFILE_ENTRY pEntry, PVOID pContext, bool *pDelete) {
			struct FLUSH_INFO	*p	= (struct FLUSH_INFO *)pContext;
			bool	bDelete	= false;

			if( p->PID ) {
				if( p->PID == pEntry->PID ) {
					bDelete	= true;				
				}			
			}
			else {
				if( ProcessTable()->IsExisting(pEntry->PID, NULL, NULL)) {
					if( (p->nTicks - pEntry->dwTicks) > 3000 ) {
						//__dlog("%d %p %d %I64d %d", 
						//	bDelete, pEntry->RegPUID, pEntry->nCount, pEntry->dwSize, 
						//	(DWORD)(p->nTicks - pEntry->dwTicks));
						bDelete	= true;
					}
				}
				else {
					bDelete	= true;
				}			
			}
			if( bDelete ) {
				p->nCount++;
				PY_FILE_MESSAGE		pMsg	= NULL;
				CreateFileMessage(pEntry, &pMsg);
				if( pMsg ) {
					if (MessageThreadPool()->Push(__FUNCTION__,
						pMsg->mode,
						pMsg->category,
						pMsg, pMsg->wDataSize + pMsg->wStringSize, false))
					{
						
					}
					else {
						CMemory::Free(pMsg);
					}
				}
			}
			*pDelete	= bDelete;
		});
		if( info.nCount )
			MessageThreadPool()->Alert(YFilter::Message::Category::Registry);
		return info.nCount;
	}
	static	void	Thread(PVOID pContext) {
		CFileTable	*pClass	= (CFileTable *)pContext;
		pClass->Flush(NULL);
	}
	void			Destroy()
	{
		__log(__FUNCTION__);
		if (m_bInitialize) {
			m_thread.Destroy();
			Clear();
			m_bInitialize = false;
		}
	}
	void			Update(PFILE_ENTRY pEntry, DWORD dwSize) {
		pEntry->nCount++;
		KeQueryTickCount(&pEntry->dwTicks);
		pEntry->nSize	+= dwSize;
	
	}
	bool			Add
	(
		PVOID					pKey,
		PFILE_CALLBACK_ARG		p,
		ULONG					*pnCount
	)
	{
		if (false == IsPossible())	{
			__dlog("%-32s IsPossible()==%d", __func__, false);
			return false;
		}
		BOOLEAN			bRet = false;
		FILE_ENTRY		entry;
		PFILE_ENTRY		pEntry = NULL;
		KIRQL			irql = KeGetCurrentIrql();

		if (irql >= DISPATCH_LEVEL) {
			__dlog("%s DISPATCH_LEVEL", __func__);
			return false;
		}
		RtlZeroMemory(&entry, sizeof(FILE_ENTRY));

		entry.pKey		= pKey;
		entry.FUID		= GetFileUID(p->pFilePath);
		entry.FPUID		= GetFilePUID(p->PID, p->subType, p->pFilePath);
		entry.PID		= p->PID;
		entry.TID		= p->TID;
		entry.PUID		= p->PUID;
		entry.nCount	= 0;
		entry.nSize		= p->nSize;
		entry.pContext	= this;
		
		p->FPUID		= entry.FPUID;
		p->FUID			= entry.FUID;
		//KeQuerySystemTime(&entry.times[0]);
		KeQueryTickCount(&entry.dwTicks);
		//	TIME_FIELDS
		//RtlTimeToTimeFields;
		//__log("%-32s %wZ", __func__, p->pFilePath);
		__try 
		{
			Lock(&irql);
			//ULONG	nTableCount = RtlNumberGenericTableElements(&m_table);
			//__dlog("RegistryTable:%d", nTableCount);

			if( IsExisting(pKey, false, &entry, [](PFILE_ENTRY pEntry, PVOID pContext) {
				PFILE_ENTRY	p	= (PFILE_ENTRY)pContext;
				CFileTable	*pClass	= (CFileTable *)p->pContext;
				pClass->Update(pEntry, (DWORD)p->nSize);
			
			})) {
				//__dlog("%-32s %p IsExisting==%d", __func__, entry.PUID, true);
			
			}
			else {
				if (p->pFilePath)
				{
					if (false == CMemory::AllocateUnicodeString(PoolType(), &entry.FilePath, p->pFilePath, 'HTAP')) {
						__dlog("CMemory::AllocateUnicodeString() failed.");
						__leave;
					}
				}
				pEntry = (PFILE_ENTRY)RtlInsertElementGenericTable(&m_table, &entry, sizeof(FILE_ENTRY), &bRet);
				//	RtlInsertElementGenericTable returns a pointer to the newly inserted element's associated data, 
				//	or it returns a pointer to the existing element's data if a matching element already exists in the generic table.
				//	If no matching element is found, but the new element cannot be inserted
				//	(for example, because the AllocateRoutine fails), RtlInsertElementGenericTable returns NULL.
				//	if( bRet && pEntry ) PrintProcessEntry(__FUNCTION__, pEntry);
				//__dlog("%-32s RtlInsertElementGenericTable==%d", __func__, bRet);
				if( pEntry ) {
					pEntry->nCount++;
					//__dlog("%-32s %p", __func__, pEntry->FPUID);
				}
				if( pnCount )	*pnCount = RtlNumberGenericTableElements(&m_table);
				if( bRet ) {
					
				}
				else {
					if( pEntry ) {
						Update(pEntry, p->nSize);			
					}
				}
			}
			Unlock(irql);
		}
		__finally
		{
			if (false == bRet)
			{
				if (pEntry)
				{
					//	이미 존재하고 있는 것이다. 
					//__dlog("  %p %10d already existing", pEntry, h);
				}
				else
				{
					//	존재하지 않은 건에 대한 입력 실패
				}
				//__dlog("%10d failed. IRQL=%d", (int)h, KeGetCurrentIrql());
				FreeEntryData(&entry, false);
			}
			//if( bRet )	__dlog("%-32s %d %p %d", "CFileTable::Add", bRet, p->FPUID, Count());
		}
		return bRet;
	}
	bool			Remove(
		IN PVOID			pKey,
		IN bool				bLock,
		IN PVOID			pCallbackPtr,
		PFileTableCallback	pCallback
	)
	{
		if (false == IsPossible())
		{
			__dlog("%-32s IsPossible() == false", __func__);
			return false;
		}
		bool			bRet	= false;
		FILE_ENTRY		entry	= { 0, };
		KIRQL			irql	= KeGetCurrentIrql();

		entry.pKey	= pKey;
		if (bLock)	Lock(&irql);
		PFILE_ENTRY	pEntry = (PFILE_ENTRY)RtlLookupElementGenericTable(&m_table, &entry);
		if (pEntry)
		{
			if (pCallback)	pCallback(pEntry, pCallbackPtr);
			FreeEntryData(pEntry);
			if (RtlDeleteElementGenericTable(&m_table, pEntry))
			{
				bRet = true;
			}
			else
			{
				//__dlog("%10d can not delete.");
			}
		}
		else
		{
			__dlog("%p not existing", pKey);
			if (pCallback) pCallback(NULL, pCallbackPtr);
		}
		if (bLock)	Unlock(irql);
		//if( bRet )	__dlog("%-32s %d %p %d", "CFileTable::Remove", bRet, FPUID, Count());
		return bRet;
	}
	ULONG			Count()
	{
		if (false == IsPossible())	return 0;
		KIRQL			irql = KeGetCurrentIrql();

		if (irql >= DISPATCH_LEVEL) {
			__dlog("%s DISPATCH_LEVEL", __FUNCTION__);
			return 0;
		}
		ULONG	nCount = 0;
		Lock(&irql);
		nCount = RtlNumberGenericTableElements(&m_table);
		Unlock(irql);
		
		return nCount;
	}
	bool			IsExisting(
		IN	PVOID				pKey,
		IN	bool				bLock,
		IN	PVOID				pCallbackPtr = NULL,
		IN	PFileTableCallback	pCallback = NULL
	)
	{
		if (false == IsPossible())
		{
			__dlog("%s IsPossible() = false", __FUNCTION__);
			return false;
		}
		bool			bRet	= false;
		KIRQL			irql	= KeGetCurrentIrql();
		FILE_ENTRY		entry;
		entry.pKey	= pKey;

		if( bLock )	Lock(&irql);
		LPVOID			p = RtlLookupElementGenericTable(&m_table, &entry);
		if (p)
		{
			if (pCallback) {
				pCallback((PFILE_ENTRY)p, pCallbackPtr);
			}
			else
			{
				__log("%s pCallback is NULL.", __FUNCTION__);
			}
			bRet = true;
		}
		else
		{
			__log("%-32s %I64d not found.", __FUNCTION__, pKey);
		}
		if( bLock )	Unlock(irql);
		return bRet;
	}
	void			FreeEntryData(PFILE_ENTRY pEntry, bool bLog = false)
	{
		if (bLog)	__function_log;
		if (NULL == pEntry)	return;
		if( bLog )	__dlog("%-32s FilePath", __func__);
		CMemory::FreeUnicodeString(&pEntry->FilePath, bLog);
	}
	void			Flush(PVOID pContext, void (*pCallback)(
					PFILE_ENTRY pEntry, PVOID pContext, bool *pDelete))
	{
		//__dlog("%s IRQL=%d", __FUNCTION__, KeGetCurrentIrql());
		if (false == IsPossible() || false == m_bInitialize)
		{
			return;
		}
		KIRQL		irql;
		PFILE_ENTRY	pEntry = NULL;
		ULONG		nCount	= 0;
		Lock(&irql);
		for (ULONG i = 0; i < RtlNumberGenericTableElements(&m_table); )
		{
			pEntry	= (PFILE_ENTRY)RtlGetElementGenericTable(&m_table, i);
			if( NULL == pEntry )	break;

			bool	bDelete;
			pCallback(pEntry, pContext, &bDelete);
			if( bDelete ) {
				FreeEntryData(pEntry, false);
				RtlDeleteElementGenericTable(&m_table, pEntry);
				nCount++;
				//__dlog("%-32s %p deleted", __func__, pEntry->RegPUID);
			}
			else {
				i++;
			}
		}
		nCount	= RtlNumberGenericTableElements(&m_table);
		if( nCount > 100 )
			__dlog("%-32s T:%d D:%d", "CFileTable::Flush", RtlNumberGenericTableElements(&m_table), nCount);
		Unlock(irql);
	}
	void			Clear()
	{
		//__dlog("%s IRQL=%d", __FUNCTION__, KeGetCurrentIrql());
		if (false == IsPossible() || false == m_bInitialize)
		{
			return;
		}
		KIRQL		irql;
		PFILE_ENTRY	pEntry = NULL;
		Lock(&irql);
		while (!RtlIsGenericTableEmpty(&m_table))
		{
			pEntry = (PFILE_ENTRY)RtlGetElementGenericTable(&m_table, 0);
			if (pEntry)
			{
				__dlog("%-32s %p", __func__, pEntry->FPUID);
				//__dlog("  path    %p(%d)", pEntry->path.Buffer, pEntry->path.Length);
				//__dlog("  command %p(%d)", pEntry->command.Buffer, pEntry->command.Length);
				FreeEntryData(pEntry, false);
				RtlDeleteElementGenericTable(&m_table, pEntry);
			}
		}
		Unlock(irql);
	}
	void			Lock(OUT KIRQL* pOldIrql)
	{
		/*
			KeAcquireSpinLock first resets the IRQL to DISPATCH_LEVEL and then acquires the lock.
			The previous IRQL is written to OldIrql after the lock is acquired.

			The code within a critical region guarded by an spin lock must neither be pageable nor make any references to pageable data.
			The code within a critical region guarded by a spin lock can neither call any external function
			that might access pageable data or raise an exception, nor can it generate any exceptions.
			The caller should release the spin lock with KeReleaseSpinLock as quickly as possible.
			https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-keacquirespinlock
		*/
		if (false == m_bInitialize)	return;
		if (m_bSupportDIRQL)	KeAcquireSpinLock(&m_lock, pOldIrql);
		else
			ExAcquireFastMutex(&m_mutex);
	}
	void			Unlock(IN KIRQL oldIrql)
	{
		if (false == m_bInitialize)	return;
		if (m_bSupportDIRQL)	KeReleaseSpinLock(&m_lock, oldIrql);
		else
			ExReleaseFastMutex(&m_mutex);
	}
private:
	bool				m_bInitialize;
	bool				m_bSupportDIRQL;
	RTL_GENERIC_TABLE	m_table;
	FAST_MUTEX			m_mutex;
	KSPIN_LOCK			m_lock;
	POOL_TYPE			m_pooltype;
	CThread				m_thread;

	POOL_TYPE		PoolType()
	{
		//__dlog("%-32s %d", __func__, m_pooltype);
		return m_pooltype;
	}
	bool			IsPossible()
	{
		if (false == m_bInitialize)	return false;
		if (false == m_bSupportDIRQL &&
			KeGetCurrentIrql() >= DISPATCH_LEVEL) {
			DbgPrintEx(0, 0, "%s return false.\n", __FUNCTION__);
			return false;
		}
		return true;
	}
	static	RTL_GENERIC_COMPARE_RESULTS	NTAPI	Compare(
		struct _RTL_GENERIC_TABLE* Table,
		PVOID FirstStruct,
		PVOID SecondStruct
	)
	{
		UNREFERENCED_PARAMETER(Table);
		//CHandleTable *	pClass = (CHandleTable *)Table->TableContext;
		PFILE_ENTRY	pFirst	= (PFILE_ENTRY)FirstStruct;
		PFILE_ENTRY	pSecond = (PFILE_ENTRY)SecondStruct;
		if (pFirst && pSecond)
		{
			if (pFirst->pKey > pSecond->pKey ) {
				return GenericGreaterThan;
			}
			else if (pFirst->pKey < pSecond->pKey) {
				return GenericLessThan;
			}
		}
		return GenericEqual;
	}
	static	PVOID	NTAPI	Alloc(
		struct _RTL_GENERIC_TABLE* Table,
		__in CLONG  nBytes
	)
	{
		UNREFERENCED_PARAMETER(Table);
		CFileTable* pClass = (CFileTable*)Table->TableContext;
		PVOID	p = CMemory::Allocate(pClass->PoolType(), nBytes, 'AIDL');
		return p;
	}
	static	PVOID	NTAPI	AllocInNoDispatchLevel(
		struct _RTL_GENERIC_TABLE* Table,
		__in CLONG  nBytes
	)
	{
		UNREFERENCED_PARAMETER(Table);
		return CMemory::Allocate(PagedPool, nBytes, 'AIDL');
	}
	static	VOID	NTAPI	Free(
		struct _RTL_GENERIC_TABLE* Table,
		__in PVOID  Buffer
	)
	{
		UNREFERENCED_PARAMETER(Table);
		CMemory::Free(Buffer);
	}
};

EXTERN_C_START
CFileTable*		FileTable();
void			CreateFileTable();
void			DestroyFileTable();
EXTERN_C_END