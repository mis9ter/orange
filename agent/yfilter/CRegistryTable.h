#pragma once
#include <stdint.h>

#define TAG_OBJECT			'tjbo'

typedef	uint64_t		REGUID;

typedef struct REGISTRY_ENTRY
{
	REGUID				RegUID;

} REG_ENTRY, * PREG_ENTRY;

typedef void (*PRegTableCallback)(
	IN PREG_ENTRY			pEntry,			//	대상 엔트리 
	IN PVOID				pCallbackPtr
);

class CRegTable
{
public:
	bool	IsInitialized()
	{
		return m_bInitialize;
	}
	void	Initialize(IN bool bSupportDIRQL = true)
	{
		__log(__FUNCTION__);
		RtlZeroMemory(this, sizeof(CProcessTable));
		m_bSupportDIRQL = bSupportDIRQL;
		m_pooltype		= bSupportDIRQL ? NonPagedPoolNx : PagedPool;
		ExInitializeFastMutex(&m_mutex);
		KeInitializeSpinLock(&m_lock);
		RtlInitializeGenericTable(&m_table, Compare, Alloc, Free, this);
		m_bInitialize = true;
	}
	void	Destroy()
	{
		__log(__FUNCTION__);
		if (m_bInitialize) {
			Clear();
			m_bInitialize = false;
		}
	}
	bool		Add
	(
		IN	REGUID				RegUID,
		IN	PUNICODE_STRING		pKey,
		IN	PVOID				pCallbackPtr = NULL,
		IN	PRegTableCallback	pCallback = NULL
	)
	{
		if (false == IsPossible())	return false;
		BOOLEAN			bRet = false;
		REG_ENTRY		entry;
		PREG_ENTRY		pEntry = NULL;
		KIRQL			irql = KeGetCurrentIrql();

		if (irql >= DISPATCH_LEVEL) {
			__dlog("%s DISPATCH_LEVEL", __FUNCTION__);
			return false;
		}
		RtlZeroMemory(&entry, sizeof(REG_ENTRY));
		entry.RegUID	= RegUID;
		__try 
		{
			Lock(&irql);
			if( IsExisting(RegUID, false, pCallbackPtr, pCallback) ) {
			
			
			}
			else {
				if( pKey ) 
				{
					//if (false == CMemory::AllocateUnicodeString(PoolType(), &entry.RegPath, pKey, 'PDDA')) 
					//{
					//	__dlog("CMemory::AllocateUnicodeString() failed.");
					//	__leave;
					//}			
				}
				pEntry = (PREG_ENTRY)RtlInsertElementGenericTable(&m_table, &entry, sizeof(PREG_ENTRY), &bRet);
				//	RtlInsertElementGenericTable returns a pointer to the newly inserted element's associated data, 
				//	or it returns a pointer to the existing element's data if a matching element already exists in the generic table.
				//	If no matching element is found, but the new element cannot be inserted
				//	(for example, because the AllocateRoutine fails), RtlInsertElementGenericTable returns NULL.
				//	if( bRet && pEntry ) PrintProcessEntry(__FUNCTION__, pEntry);
				ULONG	nCount = RtlNumberGenericTableElements(&m_table);
				__dlog("RegistryTable:%d", nCount);
				//if (pEntry)	pEntry->bFree = true;
				if( pEntry ) {
					if (pCallback)
						pCallback(pEntry, pCallbackPtr);
				}
				bRet	= true;
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
		}
		return bRet;
	}
	bool		IsExisting(
		IN	REGUID					RegUID,
		IN	bool					bLock,
		IN	PVOID					pCallbackPtr = NULL,
		IN	PRegTableCallback		pCallback = NULL
	)
	{
		if (false == IsPossible())
		{
			__dlog("%s IsPossible() = false", __FUNCTION__);
			return false;
		}
		bool			bRet	= false;
		KIRQL			irql	= KeGetCurrentIrql();
		REG_ENTRY		entry;
		entry.RegUID	= RegUID;

		if( bLock )	Lock(&irql);
		LPVOID			p = RtlLookupElementGenericTable(&m_table, &entry);
		if (p)
		{
			if (pCallback) {
				pCallback((PREG_ENTRY)p, pCallbackPtr);
			}
			else
			{
				__log("%s pCallback is NULL.", __FUNCTION__);
			}
			bRet = true;
		}
		else
		{
			__log("%-32s %I64d not found.", __FUNCTION__, RegUID);
		}
		if( bLock )	Unlock(irql);
		return bRet;
	}
	bool		Remove(
		IN REGUID			RegUID,
		IN bool				bLock,
		IN PVOID			pCallbackPtr,
		PRegTableCallback	pCallback
	)
	{
		if (false == IsPossible())
		{
			return false;
		}
		bool			bRet	= false;
		REG_ENTRY		entry	= { 0, };
		KIRQL			irql	= KeGetCurrentIrql();

		entry.RegUID	= RegUID;
		if (bLock)	Lock(&irql);
		PREG_ENTRY	pEntry = (PREG_ENTRY)RtlLookupElementGenericTable(&m_table, &entry);
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
			//__dlog("%10d not existing");
			if (pCallback) pCallback(&entry, pCallbackPtr);
		}
		if (bLock)	Unlock(irql);
		return bRet;
	}
	void		FreeEntryData(PREG_ENTRY pEntry, bool bLog = false)
	{
		if (bLog)	__function_log;
		if (NULL == pEntry)	return;
		//CMemory::FreeUnicodeString(&pEntry->RegPath, bLog);
	}
	void		Clear()
	{
		//__dlog("%s IRQL=%d", __FUNCTION__, KeGetCurrentIrql());
		if (false == IsPossible() || false == m_bInitialize)
		{
			return;
		}
		KIRQL		irql;
		PREG_ENTRY	pEntry = NULL;
		Lock(&irql);
		while (!RtlIsGenericTableEmpty(&m_table))
		{
			pEntry = (PREG_ENTRY)RtlGetElementGenericTable(&m_table, 0);
			if (pEntry)
			{
				//__dlog("%s %d", __FUNCTION__, pEntry->handle);
				//__dlog("  path    %p(%d)", pEntry->path.Buffer, pEntry->path.Length);
				//__dlog("  command %p(%d)", pEntry->command.Buffer, pEntry->command.Length);
				FreeEntryData(pEntry);
				RtlDeleteElementGenericTable(&m_table, pEntry);
			}
		}
		Unlock(irql);
	}
	void		Lock(OUT KIRQL* pOldIrql)
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
	void		Unlock(IN KIRQL oldIrql)
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

	POOL_TYPE	PoolType()
	{
		__dlog("%-32s %d", __func__, m_pooltype);
		return m_pooltype;
	}
	bool	IsPossible()
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
		PREG_ENTRY	pFirst	= (PREG_ENTRY)FirstStruct;
		PREG_ENTRY	pSecond = (PREG_ENTRY)SecondStruct;
		if (pFirst && pSecond)
		{
			if (pFirst->RegUID > pSecond->RegUID ) {
				return GenericGreaterThan;
			}
			else if (pFirst->RegUID < pSecond->RegUID) {
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
		CRegTable* pClass = (CRegTable*)Table->TableContext;
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
CRegTable*		RegistryTable();
void			CreateRegistryTable();
void			DestroyRegistryTable();
EXTERN_C_END