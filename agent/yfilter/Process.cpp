#include "yfilter.h"
#include <wdm.h>

#define	USE_OBJECT_CALLBACK	0

typedef struct PROCESS_ENTRY
{
	HANDLE				handle;
	HANDLE				parent;
	UNICODE_STRING		path;
	UNICODE_STRING		command;
	PVOID				key;
} PROCESS_ENTRY, *PPROCESS_ENTRY;

typedef void (*PProcessTableCallback)(PPROCESS_ENTRY pEntry);

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
		m_bSupportDIRQL		= bSupportDIRQL;
		m_pooltype			= bSupportDIRQL? NonPagedPoolNx : PagedPool;
		ExInitializeFastMutex(&m_mutex);
		KeInitializeSpinLock(&m_lock);
		RtlInitializeGenericTable(&m_table, Compare, Alloc, Free, this);
		m_bInitialize		= true;
	}
	void	Destroy()
	{
		__log(__FUNCTION__);
		if( m_bInitialize ) {
			Clear();
			m_bInitialize = false;
		}		
	}
	void		PrintProcessEntry(PCSTR pName, PPROCESS_ENTRY p, bool bLog = false)
	{
		UNREFERENCED_PARAMETER(pName);
		UNREFERENCED_PARAMETER(p);
		UNREFERENCED_PARAMETER(bLog);
		if( p && bLog )
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
	bool		Add(IN HANDLE PID, IN HANDLE PPID, IN PCUNICODE_STRING pPath, IN PCUNICODE_STRING pCommand)
	{
		if (false == IsPossible())	return false;
		BOOLEAN			bRet = false;
		PROCESS_ENTRY	entry		;
		PPROCESS_ENTRY	pEntry	= NULL;
		ULONG			nCount	= 0;
		KIRQL			irql	= KeGetCurrentIrql();

		//__dlog("%s %10d IRQL=%d", __FUNCTION__, h, KeGetCurrentIrql());
		if( irql >= DISPATCH_LEVEL )	return false;

		UNREFERENCED_PARAMETER(pPath);
		UNREFERENCED_PARAMETER(pCommand);
		RtlZeroMemory(&entry, sizeof(PROCESS_ENTRY));
		entry.handle	= PID;
		entry.parent	= PPID;
		entry.key = (PVOID)4;

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
				if(NT_SUCCESS(GetProcessImagePathByProcessId(PID, (PWSTR)procPath, procPath.CbSize(), NULL)))
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
			if (pCommand)
			{
				if( false == CMemory::AllocateUnicodeString(PoolType(), &entry.command, pCommand, 'PDDA') ) {
					__dlog("CMemory::AllocateUnicodeString() failed.");
					__leave;
				}			
			}
			Lock(&irql);
			pEntry	= (PPROCESS_ENTRY)RtlInsertElementGenericTable(&m_table, &entry, sizeof(PROCESS_ENTRY), &bRet);
			//	RtlInsertElementGenericTable returns a pointer to the newly inserted element's associated data, 
			//	or it returns a pointer to the existing element's data if a matching element already exists in the generic table.
			//	If no matching element is found, but the new element cannot be inserted
			//	(for example, because the AllocateRoutine fails), RtlInsertElementGenericTable returns NULL.
			//	if( bRet && pEntry ) PrintProcessEntry(__FUNCTION__, pEntry);
			nCount	= RtlNumberGenericTableElements(&m_table);
			if( true )
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
			if( false == bRet )
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
		IN	PProcessTableCallback	pCallback	= NULL
	)
	{
		if (false == IsPossible() )
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
			if( pCallback )	{
				__log("%s calling callback", __FUNCTION__);
				pCallback((PPROCESS_ENTRY)p);
				__log("%s called  callback", __FUNCTION__);
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
	bool		Remove(IN HANDLE h, IN bool bLock, 
					PProcessTableCallback pCallback)
	{
		//__dlog("%s %10d IRQL=%d", __FUNCTION__, h, KeGetCurrentIrql());
		if (false == IsPossible() )
			return false;

		bool			bRet = false;
		PROCESS_ENTRY	entry	= {0,};
		ULONG			nCount	= 0;
		KIRQL			irql	= 0;
		entry.handle	= h;
		if( bLock )	Lock(&irql);
		PPROCESS_ENTRY	pEntry = (PPROCESS_ENTRY)RtlLookupElementGenericTable(&m_table, &entry);
		if (pEntry)
		{
			if( pCallback )	pCallback(pEntry);
			//PrintProcessEntry(__FUNCTION__, pEntry);
			FreeEntryData(pEntry);
			if (RtlDeleteElementGenericTable(&m_table, pEntry))
			{	
				if( true )
				{
					nCount	= RtlNumberGenericTableElements(&m_table);
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
		}
		if( bLock )	Unlock(irql);
		return bRet;
	}
	void		FreeEntryData(PPROCESS_ENTRY pEntry, bool bLog = false)
	{
		if( bLog )	__function_log;
		if( NULL == pEntry )	return;
		//__log("  %wZ", __FUNCTION__, &pEntry->path);
		//__log("  %wZ", __FUNCTION__, &pEntry->command);
		CMemory::FreeUnicodeString(&pEntry->path, bLog);
		CMemory::FreeUnicodeString(&pEntry->command, bLog);
	}
	void		Clear()
	{
		//__dlog("%s IRQL=%d", __FUNCTION__, KeGetCurrentIrql());
		if( false == IsPossible() || false == m_bInitialize ) 
		{
			return;
		}
		KIRQL	irql;
		PPROCESS_ENTRY	pEntry	= NULL;
		Lock(&irql);
		while (!RtlIsGenericTableEmpty(&m_table))
		{
			pEntry	= (PPROCESS_ENTRY)RtlGetElementGenericTable(&m_table, 0);
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
	void		Lock(OUT KIRQL * pOldIrql)
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
		if( false == m_bInitialize )	return;
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
		if( false == m_bInitialize )	return false;
		if (false == m_bSupportDIRQL &&
			KeGetCurrentIrql() >= DISPATCH_LEVEL) {
			DbgPrintEx(0, 0, "%s return false.\n", __FUNCTION__);
			return false;
		}
		return true;
	}
	static	RTL_GENERIC_COMPARE_RESULTS	NTAPI	Compare(
		struct _RTL_GENERIC_TABLE *Table,
		PVOID FirstStruct,
		PVOID SecondStruct
	)
	{
		UNREFERENCED_PARAMETER(Table);
		//CHandleTable *	pClass = (CHandleTable *)Table->TableContext;
		PPROCESS_ENTRY	pFirst	= (PPROCESS_ENTRY)FirstStruct;
		PPROCESS_ENTRY	pSecond	= (PPROCESS_ENTRY)SecondStruct;
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
		struct _RTL_GENERIC_TABLE *Table,
		__in CLONG  nBytes
	)
	{
		UNREFERENCED_PARAMETER(Table);
		CProcessTable *	pClass	= (CProcessTable *)Table->TableContext;
		PVOID	p	= CMemory::Allocate(pClass->m_pooltype, nBytes, 'AIDL');
		//__dlog("%s %p %d bytes (PROCESS_ENTRY=%d)", __FUNCTION__, p, nBytes, sizeof(PROCESS_ENTRY));
		return p;
	}
	static	PVOID	NTAPI	AllocInNoDispatchLevel(
		struct _RTL_GENERIC_TABLE *Table,
		__in CLONG  nBytes
	)
	{
		UNREFERENCED_PARAMETER(Table);
		return CMemory::Allocate(PagedPool, nBytes, 'AIDL');
	}
	static	VOID	NTAPI	Free(
		struct _RTL_GENERIC_TABLE *Table,
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
} HANDLE_TABLE, *PHANDLE_TABLE;

static	CProcessTable	g_process;

CProcessTable *	ProcessTable()
{
	return &g_process;
}
void			CreateProcessTable()
{
	g_process.Initialize();
}
void			DestroyProcessTable()
{
	g_process.Destroy();
}
bool			IsRegisteredProcess(IN HANDLE h)
{
	if( false == g_process.IsInitialized() )	return false;
	return g_process.IsExisting(h);
}
bool			RegisterProcess(IN HANDLE h)
{
	return g_process.Add(h, NULL, NULL, NULL);
}
bool			DeregisterProcess(IN HANDLE h)
{
	return g_process.Remove(h, true, NULL);
}
NTSTATUS		KillProcess(HANDLE pid)
{
	if (pid <= (HANDLE)4)					return STATUS_INVALID_PARAMETER;
	if (KeGetCurrentIrql() > PASSIVE_LEVEL)	return STATUS_UNSUCCESSFUL;
	if (NULL == Config()->pZwTerminateProcess)	return STATUS_BAD_FUNCTION_TABLE;

	NTSTATUS	status = STATUS_UNSUCCESSFUL;
	HANDLE		hProcess = NULL;
	__try
	{
		OBJECT_ATTRIBUTES oa = { 0 };
		oa.Length = sizeof(OBJECT_ATTRIBUTES);
		InitializeObjectAttributes(&oa, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);

		CLIENT_ID cid = { 0 };
		cid.UniqueProcess = pid;
		status = ZwOpenProcess(&hProcess, PROCESS_TERMINATE, &oa, &cid);
		if (NT_FAILED(status))	__leave;
		status = Config()->pZwTerminateProcess(hProcess, STATUS_SUCCESS);
	}
	__finally
	{
		if (hProcess)	ZwClose(hProcess);
	}
	return status;
}
static	bool	IsSkip(
	IN HANDLE							currentPID,
	IN HANDLE							targetPID,
	IN POB_PRE_OPERATION_INFORMATION	pOperationInformation
)
{
	bool	bRet	= true;
	__try
	{
		if ((HANDLE)4 == currentPID || currentPID == targetPID)
			__leave;
		if (KernelMode == ExGetPreviousMode())	
			__leave;
		if (!MmIsAddressValid(pOperationInformation->Object))	
			__leave;
		if (OB_OPERATION_HANDLE_CREATE != pOperationInformation->Operation)	
			__leave;
		bRet	= false;
	}
	__finally
	{

	}
	return bRet;
}
void			PrintMask(PCSTR pTitle, YFilter::Process::Context context, PACCESS_MASK pMask)
{
	if( YFilter::Process::Context::Process == context )
	{
		//	A0121410

		if( pTitle )	__dlog("%s", pTitle); __dlog("%s", __LINE);
		if (*pMask & PROCESS_TERMINATE)						__dlog("  %s", "PROCESS_TERMINATE");
		if (*pMask & PROCESS_CREATE_THREAD)					__dlog("  %s", "PROCESS_CREATE_THREAD");
		if (*pMask & PROCESS_SET_SESSIONID)					__dlog("  %s", "PROCESS_SET_SESSIONID");
		if (*pMask & PROCESS_VM_OPERATION)					__dlog("  %s", "PROCESS_VM_OPERATION");
		if (*pMask & PROCESS_VM_READ)						__dlog("  %s", "PROCESS_VM_READ");
		if (*pMask & PROCESS_VM_WRITE)						__dlog("  %s", "PROCESS_VM_WRITE");
		if (*pMask & PROCESS_DUP_HANDLE)					__dlog("  %s", "PROCESS_DUP_HANDLE");
		if (*pMask & PROCESS_CREATE_PROCESS)				__dlog("  %s", "PROCESS_CREATE_PROCESS");
		if (*pMask & PROCESS_SET_QUOTA)						__dlog("  %s", "PROCESS_SET_QUOTA");
		if (*pMask & PROCESS_SET_INFORMATION)				__dlog("  %s", "PROCESS_SET_INFORMATION");
		if (*pMask & PROCESS_QUERY_INFORMATION)				__dlog("  %s", "PROCESS_QUERY_INFORMATION");
		if (*pMask & PROCESS_SUSPEND_RESUME)				__dlog("  %s", "PROCESS_SUSPEND_RESUME");
		if (*pMask & PROCESS_QUERY_LIMITED_INFORMATION)		__dlog("  %s", "PROCESS_QUERY_LIMITED_INFORMATION");
		if (*pMask & PROCESS_SET_LIMITED_INFORMATION)		__dlog("  %s", "PROCESS_SET_LIMITED_INFORMATION");
		if (*pMask & READ_CONTROL)							__dlog("  %s", "READ_CONTROL");
		if (*pMask & SYNCHRONIZE)							__dlog("  %s", "SYNCHRONIZE");
		//if (*pMask & PROCESS_ALL_ACCESS)					__dlog("  %s", "PROCESS_ALL_ACCESS");
	}
	else
	{
		if (*pMask & THREAD_TERMINATE)						__dlog("  %s", "THREAD_TERMINATE");
		if (*pMask & THREAD_SUSPEND_RESUME)					__dlog("  %s", "THREAD_SUSPEND_RESUME");
		if (*pMask & THREAD_ALERT)							__dlog("  %s", "THREAD_ALERT");
		if (*pMask & THREAD_GET_CONTEXT)					__dlog("  %s", "THREAD_GET_CONTEXT");
		if (*pMask & THREAD_SET_CONTEXT)					__dlog("  %s", "THREAD_SET_CONTEXT");
		if (*pMask & THREAD_SET_INFORMATION)				__dlog("  %s", "THREAD_SET_INFORMATION");
		if (*pMask & THREAD_SET_LIMITED_INFORMATION)		__dlog("  %s", "THREAD_SET_LIMITED_INFORMATION");
		if (*pMask & THREAD_QUERY_LIMITED_INFORMATION)		__dlog("  %s", "THREAD_QUERY_LIMITED_INFORMATION");
		if (*pMask & THREAD_RESUME)							__dlog("  %s", "THREAD_RESUME");
	}
}
bool			CheckAccessMask(
	IN PCSTR					pTitle, 
	IN YFilter::Process::Context	context,
	IN HANDLE					hCurrentProcessId, 
	IN HANDLE					hTargetProcessId,
	PACCESS_MASK 				pDesiredMask = NULL,
	PACCESS_MASK 				pOriginalDesiredMask = NULL			
)
{
	CWSTRBuffer		currentProcPath, targetProcPath;
	bool			bSaveMe			= false;					//	내가 위험해질수 있으니 보호해주
	YFilter::Process::Type	type	= YFilter::Process::Unknown;
	UNREFERENCED_PARAMETER(pDesiredMask);
	UNREFERENCED_PARAMETER(pTitle);
	UNREFERENCED_PARAMETER(context);
	UNREFERENCED_PARAMETER(hCurrentProcessId);
	UNREFERENCED_PARAMETER(hTargetProcessId);
	UNREFERENCED_PARAMETER(pDesiredMask);
	UNREFERENCED_PARAMETER(pOriginalDesiredMask);	

	__try
	{
		if (IsRegisteredProcess(hCurrentProcessId)) {
			//	현재 프로세스가 등록된 프로세스이면 대상이 무엇이든 보호하지 않는다.
			//	등록된 프로세스의 의미: 
			//		프로세스ID로 판단. 종료되면 해당 프로세스ID는 더 이상 허용되지 않는다. 
			//	허용된 프로세스의 의미:
			//		실행경로로 판단. 종료후 다시 실행되도 계속 허용된다. 
			__leave;
		}
		if (NT_FAILED(GetProcessImagePathByProcessId(hTargetProcessId, targetProcPath,
			(ULONG)targetProcPath.CbSize(), NULL))) {
			//	구하지 못한 경우도 무슨 일이 생길지 모르니 통과
			__leave;
		}
		if (
			!IsObject(YFilter::Object::Mode::Protect, YFilter::Object::Type::Process, targetProcPath)	&&
			!IsObject(YFilter::Object::Mode::Protect, YFilter::Object::Type::File, targetProcPath)
		) {
			//	대상 프로세스가 보호 대상 프로세스도 아니고
			//	보호 대상 폴더에 존재하지도 않는다면 통과
			__leave;
		}
		//	여기서부터 대상 프로세스가 보호 대상 or 보호 대상 폴더에 존재하는 경우 
		//	지켜줘야 한다.
		//	상황에 따라 허용 여부를 결정한다.
		if (NT_FAILED(GetProcessImagePathByProcessId(hCurrentProcessId, currentProcPath,
			(ULONG)currentProcPath.CbSize(), NULL))) {
			//	실패된 이유는 모르지만 이 경우 그냥 통과시켜 줍니다.
			//	괜히 붙잡다 망할라.
			__leave;
		}

		if (IsObject(YFilter::Object::Mode::White, YFilter::Object::Type::Process, currentProcPath)) {
			//	현재 프로세스가 깨끗 담백한 놈이라면 허용
			__leave;
		}
		if (IsObject(YFilter::Object::Mode::Protect, YFilter::Object::Type::File, currentProcPath)) {
			//	현재 프로세스가 보호대상 폴더안에 존재한다면
			//	같은 식구니까 허용
			__leave;
		}
		if (IsObject(YFilter::Object::Mode::Allow, YFilter::Object::Type::Process, currentProcPath)) {
			//	현재 프로세스가 허용된 프로세스라면 통과
			__leave;
		}
		bSaveMe	= true;
		if (IsObject(YFilter::Object::Mode::Gray, YFilter::Object::Type::Process, currentProcPath)) {
			//	gray는 접근은 되지만 종료, 서스펜드, VM 관련 동작을 막는다. 
			type	= YFilter::Process::Type::Gray;
			__leave;
		}
		if (IsObject(YFilter::Object::Mode::Black, YFilter::Object::Type::Process, currentProcPath)) {
			//	블랙이는 사살시킨다. 
			type = YFilter::Process::Type::Black;
			__leave;
		}
		WCHAR *pName = wcsrchr((PWSTR)currentProcPath, L'\\');
		if (pName)
		{
			pName++;
			__log(__LINE);
			__log("%ws", pName);
			OtList(ObjectTable(YFilter::Object::Mode::Black, YFilter::Object::Type::Process));
			__log(__LINE);
			if( IsObject(YFilter::Object::Mode::White, YFilter::Object::Type::Process, pName)) 
				__leave;
			if (IsObject(YFilter::Object::Mode::Gray, YFilter::Object::Type::Process, pName)) {
				type = YFilter::Process::Type::Gray;
				__leave;
			}
			if (IsObject(YFilter::Object::Mode::Black, YFilter::Object::Type::Process, pName)) {
				type = YFilter::Process::Type::Black;
				__leave;
			}
		}
	}
	__finally
	{
		if (bSaveMe && YFilter::Process::Process == context)
		{
			__log("%s", __LINE);
			__log("%s(%d)", pTitle, context);
			__log("current: %ws(%d)", (PWSTR)currentProcPath, type);
			__log("target : %ws", (PWSTR)targetProcPath);
		}
		if (YFilter::Process::Type::Unknown == type)
		{
			//	아무 관계가 없어요. 소 닭 보듯.
		}
		else if ( YFilter::Process::Type::Black == type )
		{
			if ((Config()->version.dwMajorVersion >= 6 && Config()->version.dwMinorVersion >= 3) || 
				Config()->version.dwMajorVersion >= 10)
			{
				//	죽입시다. 안그럼 날 죽일 수도 있는 놈. 정당방위
				__log("KILLING:%ws", (PCWSTR)currentProcPath);
				//KillProcess(hCurrentProcessId);
			}
		}
		else if (YFilter::Process::Type::Gray == type)
		{
			//	날 죽이지도 내 VM을 읽을 수도 없게 한다.
		}
		else
		{
			if (bSaveMe)
			{
				if (YFilter::Process::Context::Process == context)
				{
					
				}
				else
				{

				}
			}
		}
	}
	return bSaveMe;
}
void		DeleteAccessMask(PACCESS_MASK pOriginalMask, PACCESS_MASK pDesiredMask, ULONG notDesiredAccess[], WORD wCount)
{
	for (auto i = 0; i < wCount; ++i)
	{
		if (FlagOn(*pOriginalMask, notDesiredAccess[i]))
		{
			*pDesiredMask &= ~notDesiredAccess[i];
		}
	}
}
OB_PREOP_CALLBACK_STATUS	ProcessObjectPreCallback(
	PVOID							pRegistrationContext,
	POB_PRE_OPERATION_INFORMATION	pOperationInformation
)
{
	//	This routine is called at PASSIVE_LEVEL in an arbitrary thread context 
	//	with normal kernel APCs disabled. 
	//	Special kernel APCs are not disabled. For more information about APCs
	UNREFERENCED_PARAMETER(pRegistrationContext);
	UNREFERENCED_PARAMETER(pOperationInformation);
	bool					bCheck	= false;
	__try
	{
		HANDLE	currentPID = PsGetCurrentProcessId();
		HANDLE	targetPID = PsGetProcessId((PEPROCESS)pOperationInformation->Object);

		if( IsSkip(currentPID, targetPID, pOperationInformation) )		__leave;

		ULONG	notDesiredAccesses[] = { PROCESS_TERMINATE, /*PROCESS_VM_WRITE*/ };
		//ULONG	notInject[]	= { PROCESS_VM_READ};
		for (int i = 0; i < sizeof(notDesiredAccesses) / sizeof(ULONG); ++i)
		{
			if (FlagOn(pOperationInformation->Parameters->CreateHandleInformation.OriginalDesiredAccess, notDesiredAccesses[i]))
			{
				bCheck = true;
				break;
			}
		}
		if( false == bCheck )	__leave;
		if (CheckAccessMask(__FUNCTION__, YFilter::Process::Context::Process, currentPID, targetPID,
			&pOperationInformation->Parameters->CreateHandleInformation.DesiredAccess,
			&pOperationInformation->Parameters->CreateHandleInformation.OriginalDesiredAccess))
		{
			__log("pRegistrationContext    %p", pRegistrationContext);
			__log("pOperationInformation   %p", pOperationInformation);
			if( pOperationInformation ) {
				__log("  Operation             %d", pOperationInformation->Operation);
				__log("  Flags                 %d", pOperationInformation->Flags);
				__log("    KernelHandle        %d", pOperationInformation->KernelHandle);
				__log("    Reserved            %d", pOperationInformation->Reserved);
				__log("  Object                %d", pOperationInformation->Object);
				__log("  ObjectType            %d", pOperationInformation->ObjectType);
				__log("  CallContext           %d", pOperationInformation->CallContext);
				__log("  Parameters            %d", pOperationInformation->Parameters);
				if (pOperationInformation->Parameters)
				{
					__log("    CreateHandleInfor.. %d", pOperationInformation->Parameters->CreateHandleInformation);
					__log("    DuplicateHandleIn.. %d", pOperationInformation->Parameters->DuplicateHandleInformation);
				}
			}
			PrintMask("OriginalDesiredAccess", YFilter::Process::Context::Process, 
				&pOperationInformation->Parameters->CreateHandleInformation.OriginalDesiredAccess);
			//if (FALSE = pOperationInformation->KernelHandle)
			{
				DeleteAccessMask(&pOperationInformation->Parameters->CreateHandleInformation.OriginalDesiredAccess, 
					&pOperationInformation->Parameters->CreateHandleInformation.DesiredAccess, 
					notDesiredAccesses, _countof(notDesiredAccesses));
			}
			PrintMask("DesiredAccess", YFilter::Process::Context::Process, 
				&pOperationInformation->Parameters->CreateHandleInformation.DesiredAccess);
		}
	}
	__finally
	{

	}
	return OB_PREOP_SUCCESS;
}
OB_PREOP_CALLBACK_STATUS
ThreadObjectPreCallback(
	PVOID							pRegistrationContext,
	POB_PRE_OPERATION_INFORMATION	pOperationInformation
)
{
	UNREFERENCED_PARAMETER(pRegistrationContext);
	UNREFERENCED_PARAMETER(pOperationInformation);

	ULONG					notDesiredAccesses[] = { THREAD_TERMINATE, THREAD_SUSPEND_RESUME };
	bool					bCheck = false;
	__try
	{
		HANDLE	currentPID = PsGetCurrentProcessId();
		HANDLE	targetPID = PsGetThreadProcessId((PETHREAD)pOperationInformation->Object);
		if (IsSkip(currentPID, targetPID, pOperationInformation))	__leave;

		for (int i = 0; i < sizeof(notDesiredAccesses) / sizeof(ULONG); ++i)
		{
			if (FlagOn(pOperationInformation->Parameters->CreateHandleInformation.OriginalDesiredAccess, notDesiredAccesses[i]))
			{
				bCheck = true;
				break;
			}
		}
		if (false == bCheck)	__leave;

		if( CheckAccessMask(__FUNCTION__, YFilter::Process::Context::Thread, currentPID, targetPID,
			&pOperationInformation->Parameters->CreateHandleInformation.DesiredAccess,
			&pOperationInformation->Parameters->CreateHandleInformation.OriginalDesiredAccess) )
		{
			{
				DeleteAccessMask(&pOperationInformation->Parameters->CreateHandleInformation.OriginalDesiredAccess,
					&pOperationInformation->Parameters->CreateHandleInformation.DesiredAccess,
					notDesiredAccesses, _countof(notDesiredAccesses));
			}
		}	
	}
	__finally
	{

	}
	return OB_PREOP_SUCCESS;
}
wchar_t *	wcsistr(const wchar_t *String, const wchar_t *Pattern)
{
	wchar_t *pptr, *sptr, *start;

	for (start = (wchar_t *)String; *start != NULL; ++start)
	{
		while (((NULL != *start) &&
			(RtlUpcaseUnicodeChar(*start) != RtlUpcaseUnicodeChar(*Pattern))))
		{
			++start;
		}
		if (NULL == *start)
			return NULL;
		pptr = (wchar_t *)Pattern;
		sptr = (wchar_t *)start;
		while (RtlUpcaseUnicodeChar(*sptr) == RtlUpcaseUnicodeChar(*pptr))
		{
			sptr++;
			pptr++;
			if (NULL == *pptr)
				return (start);
		}
	}
	return NULL;
}
#define GINSTALLER_SIGNATURE	L"\\$GNPACK_"
inline	bool	IsGenianInstaller(IN PCWSTR pProcPath)
{
	if( pProcPath )
	{
		//	[TODO] 자체 보호로 인해 업데이트, 설치가 되지 않는다면?
		//	\??\C:\Users\TigerJ\APPDATA\LOCAL\TEMP\$GNPACK_3B99496A$\Installer.exe
		//	이와 같은 경로로 실행되면 우리 회사의 설치본이 동작한 것이므로 
		//	자동으로 허용 목록에 넣어준다. 
		//	원래 구상한 방법은 Installer.exe를 수정, Installer.exe가 스스로 자체보호에 
		//	등록하는 것이나 과도기적 단계에서 Installer.exe가 변경되지 않은 경우를 
		//	고려해서 한시적으로 이와 같은 홀을 두도록 한다.
		if (wcsistr(pProcPath, GINSTALLER_SIGNATURE))
		{
			__log("%s GENIAN_INSTALLER %ws", __FUNCTION__, pProcPath);
			return true;
		}
	}
	return false;
}
void	__stdcall	ProcessNotifyCallbackRoutineEx(
	_Inout_		PEPROCESS Process,
	_In_		HANDLE ProcessId,
	_Inout_opt_ PPS_CREATE_NOTIFY_INFO CreateInfo
)
{
	UNREFERENCED_PARAMETER(Process);
	UNREFERENCED_PARAMETER(ProcessId);
	UNREFERENCED_PARAMETER(CreateInfo);

	if( KeGetCurrentIrql() > PASSIVE_LEVEL )
		__function_log;

	CFltObjectReference		filter(Config()->pFilter);
	if (filter.Reference())
	{
		if (CreateInfo)
		{
			//	[TODO] 자체 보호로 인해 업데이트, 설치가 되지 않는다면?
			//	\??\C:\Users\TigerJ\APPDATA\LOCAL\TEMP\$GNPACK_3B99496A$\Installer.exe
			//	이와 같은 경로로 실행되면 우리 회사의 설치본이 동작한 것이므로 
			//	자동으로 허용 목록에 넣어준다. 
			//	원래 구상한 방법은 Installer.exe를 수정, Installer.exe가 스스로 자체보호에 
			//	등록하는 것이나 과도기적 단계에서 Installer.exe가 변경되지 않은 경우를 
			//	고려해서 한시적으로 이와 같은 홀을 두도록 한다.
			//CUnicodeString	path(CreateInfo->ImageFileName, PagedPool);
			//CUnicodeString	command(CreateInfo->CommandLine, PagedPool);
			//__log("%s %10d %wZ %wZ", __FUNCTION__, ProcessId, 
			//	CreateInfo->ImageFileName, CreateInfo->CommandLine);
			//if (IsGenianInstaller(CreateInfo->ImageFileName))
			//{

			//}
			CUnicodeString		procPath(CreateInfo->ImageFileName, PagedPool);
			CUnicodeString		command(CreateInfo->CommandLine, PagedPool);

			if( CreateInfo->ImageFileName )
			{
				__log("RUN %d %d %wZ %wZ", ProcessId, CreateInfo->ParentProcessId, CreateInfo->ImageFileName, CreateInfo->CommandLine);
				ProcessTable()->Add(ProcessId, CreateInfo->ParentProcessId, CreateInfo->ImageFileName, CreateInfo->CommandLine);
			}
			else
			{
				__log("CreateInfo->ImageFileName is null.");
			}
		}
		else
		{
			ProcessTable()->Remove(ProcessId, true, [](PPROCESS_ENTRY pEntry) {
				if (pEntry)
				{
					__dlog("Remove    : %d", (int)pEntry->handle);
				}
			});
		}
	}
}
bool		StartProcessFilter()
{
	__log("%s", __FUNCTION__);
	bool		bRet = false;
	NTSTATUS	status = STATUS_SUCCESS;
	if( NULL == Config() )	return false;

	CreateProcessTable();
	if( USE_OBJECT_CALLBACK )
	{
		OB_OPERATION_REGISTRATION	obOpRegs[2] = { 0, };
		obOpRegs[0].ObjectType = PsProcessType;
		obOpRegs[0].Operations = OB_OPERATION_HANDLE_CREATE;
		obOpRegs[0].PreOperation = ProcessObjectPreCallback;

		obOpRegs[1].ObjectType = PsThreadType;
		obOpRegs[1].Operations = OB_OPERATION_HANDLE_CREATE;
		obOpRegs[1].PreOperation = ThreadObjectPreCallback;

		OB_CALLBACK_REGISTRATION	obCbReg = { 0, };
		obCbReg.Version = OB_FLT_REGISTRATION_VERSION;
		obCbReg.OperationRegistrationCount = 2;
		obCbReg.OperationRegistration = obOpRegs;
		obCbReg.RegistrationContext = NULL;

		LARGE_INTEGER		lTickCount = { 0, };
		WCHAR				szAltitude[40] = L"";

		KeQueryTickCount(&lTickCount);
		RtlStringCbPrintfW(szAltitude, sizeof(szAltitude), L"%ws.%d", DRIVER_STRING_ALTITUDE, lTickCount.LowPart);
		RtlInitUnicodeString(&obCbReg.Altitude, szAltitude);

		status = Config()->pObRegisterCallbacks(&obCbReg, (PVOID *)&Config()->pObCallbackHandle);
		if (NT_SUCCESS(status))	bRet = true;
		else
		{
			STATUS_ACCESS_DENIED;
			__log("[%s:%d][FAILED] ObRegisterCallbacks status %x ", __FUNCTION__, __LINE__, status);
		}
	}
	status = PsSetCreateProcessNotifyRoutineEx(ProcessNotifyCallbackRoutineEx, FALSE);
	if (NT_SUCCESS(status))
	{
		Config()->bProcessNotifyCallback	= true;
	}
	else
	{
		__log("[%s:%d][FAILED] PsSetCreateProcessNotifyRoutineEx status %x ", __FUNCTION__, __LINE__, status);
	}
	return bRet;
}
void		StopProcessFilter(void)
{
	__log("%s", __FUNCTION__);
	if( NULL == Config() )	return;
	if( Config()->bProcessNotifyCallback ) {
		PsSetCreateProcessNotifyRoutineEx(ProcessNotifyCallbackRoutineEx, TRUE);
		Config()->bProcessNotifyCallback	= false;
	}
	if (USE_OBJECT_CALLBACK)
	{
		if (Config()->pObRegisterCallbacks && Config()->pObUnRegisterCallbacks)
		{
			if (Config()->pObCallbackHandle)
			{
				Config()->pObUnRegisterCallbacks(Config()->pObCallbackHandle);
				Config()->pObCallbackHandle = NULL;
			}
		}
	}
	DestroyProcessTable();
}