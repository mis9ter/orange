#pragma once

#include "IService.h"
#include "yagent.string.h"
#define		__CSERVICE_VERSION		"3.0.0.1"
#define		__CSERVICE_RELEASEDATE	"2011/09/22"
#define		__CSERVICE_NAME			"IService"

class CService 
	:	
	public	IService,
	public	CAppLog
{
public:
	virtual ~CService();
	//////////////////////////////////////////////////////////////////////////////
	// The Virtual functions of XModule
	//////////////////////////////////////////////////////////////////////////////
	void		Release() {
		delete this;
	}
	LPCSTR		__stdcall	Version() 
	{ 
		static	CHAR	szValue[]	= __CSERVICE_VERSION;
		return szValue;
	}
	LPCSTR		__stdcall	ReleaseDate()
	{
		static	CHAR	szValue[]	= __CSERVICE_RELEASEDATE;
		return szValue;
	}
	LPCSTR		__stdcall	BuildDate()
	{
		static	CHAR	szValue[]	= __DATE__ __TIME__;
		return szValue;
	}
	LPCSTR		__stdcall	Name()
	{
		static	CHAR	szValue[]	= __CSERVICE_NAME;
		return szValue;
	}
	///////////////////////////////////////////////////////////////////////////////
	//	CService 사용 방식
	//	1. CAgent가 있는 경우 CAgent에 상속시켜서 사용하자. 
	//	2. CService 단독으로 사용하는 경우 
	static CService *		GetInstance();
	bool	__stdcall		IsInstalled(IN LPCSTR pCause);
	bool	__stdcall		IsRunning(IN LPCSTR pCause);
	bool	__stdcall		Start(IN LPCSTR pCause);
	bool	__stdcall		StartByUser(IN LPCSTR pCause);
	bool	__stdcall		Install(IN DWORD dwStartType, IN LPCSTR pCause);
	bool	__stdcall		Remove(IN LPCSTR pCause);
	bool	__stdcall		Shutdown(IN LPCSTR pCause);

	void	__stdcall		SetServicePath(IN LPCTSTR pValue);
	void	__stdcall		SetServiceName(IN LPCTSTR lpValue);
	void	__stdcall		SetServiceDisplayName(IN LPCTSTR lpValue);
	void	__stdcall		SetServiceDescription(IN LPCTSTR lpValue);
	void	__stdcall		SetWaitHint(DWORD dwHint);
	void	__stdcall		SetExitCode(DWORD dwCode);
	bool	__stdcall		SetStatus(DWORD dwState);
	void	__stdcall		SetServiceFunction(
		IN	_ServiceFunction	pFunction, 
		IN	PVOID				pContext
	);
	void	__stdcall		SetServiceHandler(
		IN	_ServiceHandler		pHandler,
		IN	PVOID				pContext
	);
	bool	__stdcall		Console();
	bool	__stdcall		RequestByUser(DWORD dwControl);
	void	__stdcall		SetPeriod(IN DWORD dwSeconds)
	{
		m_dwPeriod	= dwSeconds;
	}
	LPCTSTR	__stdcall		GetServiceName();
	LPCTSTR	__stdcall		GetServiceDisplayName();
	LPCTSTR __stdcall		GetServiceDescription();
	LPCTSTR	__stdcall		GetServicePath();

	bool	__stdcall		GetSCMServicePath(OUT LPTSTR pValue, IN DWORD dwSize);
	bool					GetServiceConfig(IN LPCTSTR pName, OUT SERVICE_CONFIG * p);
	bool					SetServiceRecoveryMode(IN bool bEnable);

	static	LPCSTR	GetEventType(IN DWORD dwEventType)
	{
		/*
		#define PBT_APMQUERYSUSPEND             0x0000
		#define PBT_APMQUERYSTANDBY             0x0001

		#define PBT_APMQUERYSUSPENDFAILED       0x0002
		#define PBT_APMQUERYSTANDBYFAILED       0x0003

		#define PBT_APMSUSPEND                  0x0004
		#define PBT_APMSTANDBY                  0x0005

		#define PBT_APMRESUMECRITICAL           0x0006
		#define PBT_APMRESUMESUSPEND            0x0007
		#define PBT_APMRESUMESTANDBY            0x0008

		#define PBTF_APMRESUMEFROMFAILURE       0x00000001

		#define PBT_APMBATTERYLOW               0x0009
		#define PBT_APMPOWERSTATUSCHANGE        0x000A

		#define PBT_APMOEMEVENT                 0x000B


		#define PBT_APMRESUMEAUTOMATIC          0x0012
		#if (_WIN32_WINNT >= 0x0502)
		#ifndef PBT_POWERSETTINGCHANGE
		#define PBT_POWERSETTINGCHANGE          0x8013
		*/
		static	const	char	*pNames[] = {
			"PBT_APMQUERYSUSPEND",			//	0
			"PBT_APMQUERYSTANDBY",			//	1
			"PBT_APMQUERYSUSPENDFAILED",	//	2
			"PBT_APMQUERYSTANDBYFAILED",	//	3
			"PBT_APMSUSPEND",				//	4
			"PBT_APMSTANDBY",				//	5
			"PBT_APMRESUMECRITICAL",		//	6
			"PBT_APMRESUMESUSPEND",			//	7
			"PBT_APMRESUMESTANDBY",			//	8
			"PBT_APMBATTERYLOW",			//	9
			"PBT_APMPOWERSTATUSCHANGE",		//	10
			"PBT_APMOEMEVENT",				//	11
			"PBT_APMRESUMEAUTOMATIC",		//	12
			"PBT_POWERSETTINGCHANGE"		//	13
		};
		static	const	char	*pNull	= "";

		if( dwEventType >= PBT_APMQUERYSUSPEND	&&
			dwEventType <= PBT_APMOEMEVENT )
			return pNames[dwEventType];
		if( PBT_APMRESUMEAUTOMATIC == dwEventType )	
			return pNames[12];
		if(PBT_POWERSETTINGCHANGE == dwEventType )
			return pNames[13];
		return pNull;
	}
	static	LPCSTR	GetControlName(IN DWORD dwControl)
	{
		/*
		#define SERVICE_CONTROL_STOP                   0x00000001
		#define SERVICE_CONTROL_PAUSE                  0x00000002
		#define SERVICE_CONTROL_CONTINUE               0x00000003
		#define SERVICE_CONTROL_INTERROGATE            0x00000004
		#define SERVICE_CONTROL_SHUTDOWN               0x00000005
		#define SERVICE_CONTROL_PARAMCHANGE            0x00000006
		#define SERVICE_CONTROL_NETBINDADD             0x00000007
		#define SERVICE_CONTROL_NETBINDREMOVE          0x00000008
		#define SERVICE_CONTROL_NETBINDENABLE          0x00000009
		#define SERVICE_CONTROL_NETBINDDISABLE         0x0000000A
		#define SERVICE_CONTROL_DEVICEEVENT            0x0000000B
		#define SERVICE_CONTROL_HARDWAREPROFILECHANGE  0x0000000C
		#define SERVICE_CONTROL_POWEREVENT             0x0000000D
		#define SERVICE_CONTROL_SESSIONCHANGE          0x0000000E
		#define SERVICE_CONTROL_PRESHUTDOWN            0x0000000F
		#define SERVICE_CONTROL_TIMECHANGE             0x00000010
		#define SERVICE_CONTROL_TRIGGEREVENT           0x00000020
		*/

		static	const	char	pNull[]	= "";
		static	const	char	*pNames[] = {
			"SERVICE_CONTROL_INIT",						//	0
			"SERVICE_CONTROL_STOP",						//	1
			"SERVICE_CONTROL_PAUSE",					//	2
			"SERVICE_CONTROL_CONTINUE",					//	3
			"SERVICE_CONTROL_INTERROGATE",				//	4
			"SERVICE_CONTROL_SHUTDOWN",					//	5
			"SERVICE_CONTROL_PARAMCHANGE",				//	6
			"SERVICE_CONTROL_NETBINDADD",				//	7
			"SERVICE_CONTROL_NETBINDREMOVE",			//	8
			"SERVICE_CONTROL_NETBINDENABLE",			//	9
			"SERVICE_CONTROL_NETBINDDISABLE",			//	A
			"SERVICE_CONTROL_DEVICEEVENT",				//	B
			"SERVICE_CONTROL_HARDWAREPROFILECHANGE",	//	C
			"SERVICE_CONTROL_POWEREVENT",				//	D
			"SERVICE_CONTROL_SESSIONCHANGE",			//	E
			"SERVICE_CONTROL_PRESHUTDOWN",				//	F
			"SERVICE_CONTROL_TIMECHANGE",				//	10
			"SERVICE_CONTROL_TRIGGEREVENT",				//	11

		};
		if( dwControl > SERVICE_CONTROL_TRIGGEREVENT )	return pNull;
		return pNames[dwControl];
	}

private:
	CService();

	static	CService		*m_pInstance;

	DWORD					m_dwMode;			// Service Mode: service/console

	TCHAR					m_szPath[MBUFSIZE];
	TCHAR					m_szName[MBUFSIZE];
	TCHAR					m_szDisplayName[MBUFSIZE];
	TCHAR					m_szDescription[MBUFSIZE];

	SERVICE_STATUS_HANDLE	m_hStatus;
	SERVICE_STATUS			m_status;
	DWORD					m_dwError;

	DWORD					m_dwPeriod;

	_ServiceHandler			m_pServiceHandler;
	_ServiceFunction		m_pServiceFunction;
	void					*m_pServiceHandlerContext;
	void					*m_pServiceFunctionContext;


	bool					ServiceFunction(IN DWORD dwFunction, DWORD dwControl)
	{
		if( m_pServiceFunction ) {
			return m_pServiceFunction(dwFunction, dwControl, m_pServiceFunctionContext);
		}
		return false;
	}
	static	void	WINAPI	ServiceHandler(IN DWORD dwControl);
	static	DWORD	WINAPI	ServiceHandlerEx(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext);
	static	void	WINAPI	ServiceMain(DWORD ac, PTSTR *av);

};
