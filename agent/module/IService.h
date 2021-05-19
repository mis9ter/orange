#pragma once

#include "IObject.h"

#define	SERVICE_CONTROL_USER			128
#define	SERVICE_CONTROL_USER_SHUTDOWN	(SERVICE_CONTROL_USER+1)
#define SERVICE_MODE					0
#define CONSOLE_MODE					1
#define DEFAULT_PERIOD					1

#define SERVICE_CONTROL_INITIALIZE		0
#define SERVICE_FUNCTION_INITIALIZE		0
#define SERVICE_FUNCTION_RUN			1
#define SERVICE_FUNCTION_PRESHUTDOWN	2
#define SERVICE_FUNCTION_SHUTDOWN		3

typedef struct SERVICE_CONFIG
{
	DWORD	dwServiceType;
	DWORD	dwStartType;
	DWORD	dwErrorControl;
	DWORD	dwTagId;

	TCHAR	szCommand[1024];
	TCHAR	szFilePath[1024];
	TCHAR	szPath[1024];
	TCHAR	szAccount[64];
	TCHAR	szDisplayName[256];

} SERVICE_CONFIG;

typedef void (CALLBACK *_ServiceHandler)(IN DWORD dwControl, IN DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext);
typedef bool (CALLBACK *_ServiceFunction)(
	IN	DWORD		dwFunction,
	IN	DWORD		dwControl, 
	IN	void		*param
);

interface IService : public IObject
{
	virtual	bool	__stdcall		IsInstalled(IN LPCSTR pCause)	= NULL;
	virtual	bool	__stdcall		IsRunning(IN LPCSTR pCause)		= NULL;
	virtual	bool	__stdcall		Start(IN LPCSTR pCause)			= NULL;
	virtual	bool	__stdcall		StartByUser(IN LPCSTR pCause)	= NULL;
	virtual	bool	__stdcall		Install(IN DWORD dwStartType, IN LPCSTR pCause)		= NULL;
	virtual	bool	__stdcall		Remove(IN LPCSTR pCause)		= NULL;
	virtual	bool	__stdcall		Shutdown(IN LPCSTR pCause)		= NULL;

	virtual	void	__stdcall		SetServicePath(IN LPCTSTR pValue)	= NULL;
	virtual	void	__stdcall		SetServiceName(IN LPCTSTR lpValue)	= NULL;
	virtual	void	__stdcall		SetServiceDisplayName(IN LPCTSTR lpValue)	= NULL;
	virtual	void	__stdcall		SetServiceDescription(IN LPCTSTR lpValue)	= NULL;

	virtual	void	__stdcall		SetServiceFunction(
		IN	_ServiceFunction	pFunction,
		IN	PVOID				pContext
	)	= NULL;
	virtual	void	__stdcall		SetServiceHandler(
		IN	_ServiceHandler		pHandler,
		IN	PVOID				pContext
	)	= NULL;
	virtual	bool	__stdcall		RequestByUser(DWORD dwControl)	= NULL;

	//	2018.05.03	BEOM
	//	기존에 설치된 서비스의 실행 경로를 얻는 함수.
	//	업데이트 과정에서 서비스 파일명이 바뀐 경우도 있을 수 있더라.
	virtual	bool					GetServiceConfig(IN LPCTSTR pName, OUT SERVICE_CONFIG * p)	= NULL;
	virtual	bool	__stdcall		GetSCMServicePath(OUT LPTSTR pValue, IN DWORD dwSize)	= NULL;
	virtual	bool					SetServiceRecoveryMode(IN bool bEnable)	= NULL;
};


