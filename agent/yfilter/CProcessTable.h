#pragma once

#define TAG_PROCESS			'corp'

typedef struct PROCESS_ENTRY
{
	HANDLE				handle;
	HANDLE				parent;
	UUID				uuid;
	UNICODE_STRING		path;
	UNICODE_STRING		command;
	PVOID				key;
	bool				bFree;		//	해제 대상
	bool				bCallback;	//	콜백에 의해 수집
} PROCESS_ENTRY, * PPROCESS_ENTRY;

typedef void (*PProcessTableCallback)(
	IN bool					bCreationSaved,	//	생성이 감지되어 저장된 정보 여부
	IN PPROCESS_ENTRY		pEntry,			//	대상 엔트리 
	IN PVOID				pCallbackPtr
	);

class CProcessTable
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
		m_pooltype = bSupportDIRQL ? NonPagedPoolNx : PagedPool;
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
	/*
	void		PrintProcessEntry(PCSTR pName, PPROCESS_ENTRY p, bool bLog = false)
	{
		UNREFERENCED_PARAMETER(pName);
		UNREFERENCED_PARAMETER(p);
		UNREFERENCED_PARAMETER(bLog);
		if (p && bLog)
		{
			char	szIrql[30] = "";
			__dlog("%s", __LINE);
			__dlog("%-20s %s", pName, CDriverCommon::GetCurrentIrqlName(szIrql, sizeof(szIrql)));

			__dlog("PROCESS_ENTRY     %p", p);
			__dlog("  handle          %d", (int)p->handle);
			__dlog("  key             %d", (int)p->key);
			__dlog("  path.Buffer     %p", p->path.Buffer);
			__dlog("  path.Length     %d", p->path.Length);
			__dlog("  command.Buffer  %p", p->command.Buffer);
			__dlog("  command.Length  %d", p->command.Length);
		}
	}
	*/
	bool		Add(
		IN bool		bCallback,		// 1 콜백에 의해 수집 0 직접 수집
		IN HANDLE PID, IN HANDLE PPID,
		IN UUID* pUuid, IN PCUNICODE_STRING pPath, IN PCUNICODE_STRING pCommand)
	{
		if (false == IsPossible())	return false;
		BOOLEAN			bRet = false;
		PROCESS_ENTRY	entry;
		PPROCESS_ENTRY	pEntry = NULL;
		ULONG			nCount = 0;
		KIRQL			irql = KeGetCurrentIrql();

		//__dlog("%s %10d IRQL=%d", __FUNCTION__, h, KeGetCurrentIrql());
		if (irql >= DISPATCH_LEVEL) {
			__dlog("%s DISPATCH_LEVEL", __FUNCTION__);
			return false;
		}
		UNREFERENCED_PARAMETER(pPath);
		UNREFERENCED_PARAMETER(pCommand);
		RtlZeroMemory(&entry, sizeof(PROCESS_ENTRY));
		entry.handle = PID;
		entry.parent = PPID;
		entry.key = (PVOID)4;
		entry.bFree = false;
		entry.bCallback	= bCallback;
		if (pUuid)		RtlCopyMemory(&entry.uuid, pUuid, sizeof(entry.uuid));
		CWSTRBuffer		procPath;

		__try
		{
			if (pPath)
			{
				if (false == CMemory::AllocateUnicodeString(PoolType(), &entry.path, pPath, 'PDDA')) {
					__dlog("CMemory::AllocateUnicodeString() failed.");
					__leave;
				}
			}
			else
			{
				//	PID만 설정하는 경우 있음 - 프로세스 경로가 없으면 직접 구함. 
				if (NT_SUCCESS(GetProcessImagePathByProcessId(PID, (PWSTR)procPath, procPath.CbSize(), NULL)))
				{
					if (false == CMemory::AllocateUnicodeString(PoolType(), &entry.path, procPath)) {
						__dlog("CMemory::AllocateUnicodeString() failed.");
						__leave;
					}
				}
				else
				{
					__log("%s GetProcessImagePathByProcessId(%d) failed.", __FUNCTION__, PID);
					__leave;
				}
			}
			PS_PROTECTED_SIGNER	signer;
			if (NT_SUCCESS(GetProcessCodeSignerByProcessId(PID, &signer)))
			{
				//__log("%s %wZ signer:%d", __FUNCTION__, &entry.path, signer);
			}
			else
			{
				__log("%s GetProcessCodeSignerByProcessId() failed.", __FUNCTION__);
			}

			if (pCommand)
			{
				if (false == CMemory::AllocateUnicodeString(PoolType(), &entry.command, pCommand, 'PDDA')) {
					__dlog("CMemory::AllocateUnicodeString() failed.");
					__leave;
				}
			}
			Lock(&irql);
			pEntry = (PPROCESS_ENTRY)RtlInsertElementGenericTable(&m_table, &entry, sizeof(PROCESS_ENTRY), &bRet);
			//	RtlInsertElementGenericTable returns a pointer to the newly inserted element's associated data, 
			//	or it returns a pointer to the existing element's data if a matching element already exists in the generic table.
			//	If no matching element is found, but the new element cannot be inserted
			//	(for example, because the AllocateRoutine fails), RtlInsertElementGenericTable returns NULL.
			//	if( bRet && pEntry ) PrintProcessEntry(__FUNCTION__, pEntry);
			if (pEntry)	pEntry->bFree = true;
			nCount = RtlNumberGenericTableElements(&m_table);
			if (true)
			{
				for (ULONG i = 0; i < RtlNumberGenericTableElements(&m_table); i++)
				{
					auto	p = (PPROCESS_ENTRY)RtlGetElementGenericTable(&m_table, i);
					UNREFERENCED_PARAMETER(p);
					//__dlog("  [%d] %p %10d", i, p, p->handle);
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
		}
		return bRet;
	}
	bool		IsExisting(
		IN	HANDLE					h,
		IN	PVOID					pCallbackPtr = NULL,
		IN	PProcessTableCallback	pCallback = NULL
	)
	{
		if (false == IsPossible())
		{
			__dlog("%s IsPossible() = false", __FUNCTION__);
			return false;
		}
		bool			bRet = false;
		KIRQL			irql;
		PROCESS_ENTRY	entry;
		entry.handle = h;
		Lock(&irql);
		LPVOID			p = RtlLookupElementGenericTable(&m_table, &entry);
		if (p)
		{
			if (pCallback) {
				pCallback(true, (PPROCESS_ENTRY)p, pCallbackPtr);
			}
			else
			{
				__log("%s pCallback is NULL.", __FUNCTION__);
			}
			bRet = true;
		}
		else
		{
			__log("%s %d not found.", __FUNCTION__, h);
		}
		Unlock(irql);
		return bRet;
	}
	bool		Remove(IN HANDLE h,
		IN bool bLock,
		IN PVOID pCallbackPtr,
		PProcessTableCallback pCallback)
	{
		if (false == IsPossible())
		{
			__dlog("%s %10d IRQL=%d", __FUNCTION__, h, KeGetCurrentIrql());
			return false;
		}
		bool			bRet = false;
		PROCESS_ENTRY	entry = { 0, };
		ULONG			nCount = 0;
		KIRQL			irql = 0;
		entry.handle = h;
		if (bLock)	Lock(&irql);
		PPROCESS_ENTRY	pEntry = (PPROCESS_ENTRY)RtlLookupElementGenericTable(&m_table, &entry);
		if (pEntry)
		{
			if (pCallback)	pCallback(true, pEntry, pCallbackPtr);
			FreeEntryData(pEntry);
			if (RtlDeleteElementGenericTable(&m_table, pEntry))
			{
				if (true)
				{
					nCount = RtlNumberGenericTableElements(&m_table);
					for (ULONG i = 0; i < RtlNumberGenericTableElements(&m_table); i++)
					{
						auto	p = (PPROCESS_ENTRY)RtlGetElementGenericTable(&m_table, i);
						UNREFERENCED_PARAMETER(p);
						//__dlog("  [%d] %p %10d", i, p, p->handle);
					}
				}
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
			if (pCallback) pCallback(false, &entry, pCallbackPtr);
		}
		if (bLock)	Unlock(irql);
		return bRet;
	}
	void		FreeEntryData(PPROCESS_ENTRY pEntry, bool bLog = false)
	{
		if (bLog)	__function_log;
		if (NULL == pEntry)	return;
		//__log("  %wZ", __FUNCTION__, &pEntry->path);
		//__log("  %wZ", __FUNCTION__, &pEntry->command);
		CMemory::FreeUnicodeString(&pEntry->path, bLog);
		CMemory::FreeUnicodeString(&pEntry->command, bLog);
	}
	void		Clear()
	{
		//__dlog("%s IRQL=%d", __FUNCTION__, KeGetCurrentIrql());
		if (false == IsPossible() || false == m_bInitialize)
		{
			return;
		}
		KIRQL	irql;
		PPROCESS_ENTRY	pEntry = NULL;
		Lock(&irql);
		while (!RtlIsGenericTableEmpty(&m_table))
		{
			pEntry = (PPROCESS_ENTRY)RtlGetElementGenericTable(&m_table, 0);
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
		PPROCESS_ENTRY	pFirst = (PPROCESS_ENTRY)FirstStruct;
		PPROCESS_ENTRY	pSecond = (PPROCESS_ENTRY)SecondStruct;
		if (pFirst && pSecond)
		{
			if (pFirst->handle > pSecond->handle) {
				return GenericGreaterThan;
			}
			else if (pFirst->handle < pSecond->handle) {
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
		CProcessTable* pClass = (CProcessTable*)Table->TableContext;
		PVOID	p = CMemory::Allocate(pClass->m_pooltype, nBytes, 'AIDL');
		//__dlog("%s %p %d bytes (PROCESS_ENTRY=%d)", __FUNCTION__, p, nBytes, sizeof(PROCESS_ENTRY));
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
		//__dlog("%s %p", __FUNCTION__, Buffer);
		CMemory::Free(Buffer);
	}
};

typedef struct HANDLE_TABLE {
	RTL_GENERIC_TABLE		table;
	FAST_MUTEX				mutex;
	KSPIN_LOCK				lock;
} HANDLE_TABLE, * PHANDLE_TABLE;

CProcessTable*	ProcessTable();
void			CreateProcessTable();
void			DestroyProcessTable();
bool			IsRegisteredProcess(IN HANDLE h);
bool			RegisterProcess(IN HANDLE h);
bool			DeregisterProcess(IN HANDLE h);
NTSTATUS		KillProcess(HANDLE pid);