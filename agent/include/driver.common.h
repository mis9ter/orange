#pragma once
/*
	DbgPrint and DbgPrintEx can be called at IRQL<=DIRQL (Device IRQL).
	However, Unicode format codes (%wc and %ws) can be used only at IRQL = PASSIVE_LEVEL.
	Also, because the debugger uses interprocess interrupts (IPIs) to communicate with other processors,
	calling DbgPrint at IRQL>DIRQL can cause deadlocks.
*/
//	http://egloos.zum.com/cjhnim/v/3828165
//	%wc, %ws와 같은 wchar 관련 포멧을 사용할 땐 PASSIVE_LEVEL에서만 가능하다.
//	--
//	디버거가 붙어 있는 경우 IRQL이 예상과 다른 값이 될 수 도 있다. 
#define	__log(fmt, ...)		if( KeGetCurrentIrql() <= PASSIVE_LEVEL ) { DbgPrintEx(0, 0, fmt, ## __VA_ARGS__); DbgPrintEx(0, 0, "\n"); }
#define	__dlog(fmt, ...)	if( true ) { DbgPrintEx(0, 0, fmt, ## __VA_ARGS__); DbgPrintEx(0, 0, "\n"); }
#define	__function_log		{ char	szIrql[30] = ""; DbgPrintEx(0, 0, "%s\n",__LINE); DbgPrintEx(0, 0, "%-20s %s\n", 				__FUNCTION__, CDriverCommon::GetCurrentIrqlName(szIrql, sizeof(szIrql))); }


#define	__MAX_PATH_SIZE		1024
#define __LINE				"----------------------------------------------------------------"
#define NT_FAILED(Status) (!NT_SUCCESS(Status))

#include "driver.memory.h"
#include "CUnicodeString.h"
#include "driver.registry.h"


///////////////////////////////////////////////////////////////////////////////////////////////
//	Driver 개발시 유용한 함수들
///////////////////////////////////////////////////////////////////////////////////////////////
class CDriverCommon
{
public:
	static	bool	GetDriverValueFromRegistry
	(
		IN	PUNICODE_STRING pRegistryPath,
		IN	LPCWSTR			pValueName,
		OUT	PWSTR			pBuffer,
		IN	SIZE_T			nBufferSize
	)
	{
		bool	bRet = false;
		PKEY_VALUE_PARTIAL_INFORMATION	pKeyValue = NULL;
		if (NT_SUCCESS(CDriverRegistry::QueryValue(pRegistryPath, pValueName, &pKeyValue)))
		{
			if (NT_SUCCESS(RtlStringCbCopyNW(pBuffer, nBufferSize, (NTSTRSAFE_PWSTR)pKeyValue->Data,
				min(nBufferSize, pKeyValue->DataLength))))
				bRet = true;
			CDriverRegistry::FreeValue(pKeyValue);
		}
		return bRet;
	}
	static	char *		GetCurrentIrqlName(IN char * pName, IN size_t nSize)
	{
		KIRQL	irql = KeGetCurrentIrql();
		return GetIrqlName(irql, pName, nSize);
	}
	static	char *		GetIrqlName(IN KIRQL irql, IN char * pName, IN size_t cbSize)
	{
		static	NTSTRSAFE_PSTR	pNull = "";

		if (NULL == pName || 0 == cbSize)	return pNull;

		switch (irql)
		{
			__case_copynameA(PASSIVE_LEVEL, pName, cbSize); break;
			__case_copynameA(APC_LEVEL, pName, cbSize); break;
			__case_copynameA(DISPATCH_LEVEL, pName, cbSize); break;
		default:
			if (KeGetCurrentIrql() <= PASSIVE_LEVEL)
				RtlStringCbPrintfA(pName, cbSize, "IRQL:%d", irql);	//	PASSIVE_LEVEL
			break;
		}
		return pName;
	}
	
private:

};

class CAutoReleaseSpinLock
{
public:
	CAutoReleaseSpinLock(IN KSPIN_LOCK * pLock)
	{
		KeAcquireSpinLock(m_pLock = pLock, &m_oldIrql);
	}
	~CAutoReleaseSpinLock()
	{
		KeReleaseSpinLock(m_pLock, m_oldIrql);
	}
private:
	KIRQL			m_oldIrql;
	KSPIN_LOCK *	m_pLock;
};
class CFltObjectReference
{
public:
	CFltObjectReference(IN LPVOID pObject)
		:
		m_pObject(pObject),
		m_bReference(false)
	{

	}
	~CFltObjectReference()
	{
		if (m_bReference && m_pObject ) {
			FltObjectDereference(m_pObject);
		}
	}
	bool		Reference()
	{
		if( false == m_bReference )
		{
			if (m_pObject && NT_SUCCESS(FltObjectReference(m_pObject)))
			{
				return (m_bReference = true);
			}
		}
		return false;
	}
private:
	LPVOID			m_pObject;
	bool			m_bReference;
};
struct CFastMutexLocker {
	CFastMutexLocker(PFAST_MUTEX p) : m_pMutex(p) {
		if (m_pMutex)
			Lock(m_pMutex);
	}
	~CFastMutexLocker() {
		if (m_pMutex)
			Unlock(m_pMutex);
	}
	static	void	Lock(PFAST_MUTEX p)
	{
		ExAcquireFastMutex(p);
	}
	static	void	Unlock(PFAST_MUTEX p)
	{
		ExReleaseFastMutex(p);
	}
private:
	PFAST_MUTEX		m_pMutex;
};