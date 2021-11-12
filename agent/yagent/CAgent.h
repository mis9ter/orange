#pragma once

#pragma warning(disable:4819)

#include <Windows.h>
#include <functional>
#include <thread>
#include <memory>

#define	DB_EVENT_NAME		L"event"
#define DB_SUMMARY_NAME		L"summary"
#define DB_CONFIG_NAME		L"config"

#include "yagent.h"
#include "CPathConvertor.h"

#include "CThread.h"
#include "IFilterCtrl.h"
#include "CDialog.h"
#include "CException.h"
#include "CNotifyCenter.h"
#include "CIpc.h"
#include "CService.h"
#include "CFilePath.h"
#include "CEventCallback.h"
#include "CProtect.h"

typedef std::function<void(HANDLE hShutdown, void * pCallbackPtr)>	PFUNC_AGENT_RUNLOOP;


class CIPCCommandCallback
{
public:
	CIPCCommandCallback() {
	
	}
	~CIPCCommandCallback() {
	
	}

protected:
	static	bool	Sqllite3QueryUnknown
	(
		PVOID pContext, 
		const	Json::Value &req, 
		const	Json::Value	&header,
		Json::Value & res
	);
	bool			Sqllite3QueryKnown
	(
		PVOID pContext, 
		const	Json::Value &req, 
		const	Json::Value	&header,
		Json::Value & res
	);
	bool			RegisterListener
	(
		PVOID pContext, 
		const	Json::Value &req, 
		const	Json::Value	&header,
		Json::Value & res
	);
};

typedef struct _DB_CONFIG {
	int				nResourceID;
	std::wstring	strName;
	std::wstring	strODB;
	std::wstring	strCDB;
	CDB				cdb;
} DB_CONFIG;

typedef std::shared_ptr<DB_CONFIG>	DbConfigPtr;
typedef std::map<std::wstring, DbConfigPtr>		DbMap;



class CAgent
	:
	public	CFilterCtrl,
	public	CEventCallback,
	public	CFilePath,
	public	CNotifyCenter,
	public	CIPC,
	public	CIPCCommand,
	public	CIPCCommandCallback,
	public	CProtect
{
public:
	CAgent();
	virtual	~CAgent();

	void		SetResult(
		IN	PCSTR	pFile, PCSTR pFunction, int nLine,
		IN	Json::Value & res, IN bool bRet, IN int nCode, IN PCSTR pMsg /*utf8*/
	) {
		Json::Value	&_result	= res["result"];

		_result["ret"]	= bRet;
		_result["code"]	= nCode;
		_result["msg"]	= pMsg;
		_result["file"]	= pFile;
		_result["line"]	= nLine;
		_result["function"]	= pFunction;
	}
	CDB* Db(PCWSTR pName) {
		auto t	= m_db.find(pName);
		if( m_db.end() == t )	{
		
			Log("%-32s %ws is not found.", __func__, pName);
			return NULL;
		}
		Log("%-32s %ws %p", __func__, pName, t->second->cdb.Handle());
		return &t->second->cdb;
	}
	INotifyCenter *	NotifyCenter() {
		return dynamic_cast<INotifyCenter *>(this);
	}
	IService *		Service() {
		return dynamic_cast<IService *>(CService::GetInstance());
	}
	bool			IsInitialized() {
		return m_config.bInitialize;
	}
	bool			Install();
	void			Uninstall();
	bool			Start();
	void			Shutdown(IN DWORD dwControl);
	void			RunLoop(IN DWORD dwMilliSeconds);

	PCWSTR			AppPath() {
		return m_config.path.szApp;
	}
	PCWSTR			DumpPath() {
		return m_config.path.szDump;
	}
	PCWSTR			DataPath() {
		return m_config.path.szData;
	}
	void			SetRunLoop(void* pCallbackPtr, PFUNC_AGENT_RUNLOOP pCallback)
	{
		m_config.pRunLoopFunc	= pCallback;
		m_config.pRunLoopPtr	= pCallbackPtr;
	}
	static	void	CALLBACK	ServiceHandler
	(
		IN DWORD dwControl, IN DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext
	);
	static	bool	CALLBACK	ServiceFunction(DWORD dwFunction, DWORD dwControl, LPVOID lpContext);
	void			LogDoc(PCSTR pTitle, const Json::Value & res) {
		std::string					str;
		Json::StreamWriterBuilder	wbuilder;
		wbuilder["indentation"]	= "\t";
		str	= Json::writeString(wbuilder, res);
		Log(str.c_str());
	}

	bool			KOpenProcess(HANDLE PID, ACCESS_MASK desiredAccess, PHANDLE pProcessHandle) {
		COMMAND_OPENPROCESS		command	= {0,};
		command.dwCommand		= Y_COMMAND_OPENPROCESS;
		command.DesiredAccess	= desiredAccess;
		command.pOpenHandle		= pProcessHandle;
		command.PID				= PID;
		command.nSize			= sizeof(COMMAND_OPENPROCESS);
		if (CFilterCtrl::SendCommand2(&command)) {
			Log("%-32s %-20s %d", __func__, "Y_COMMAND_OPENPROCESS", command.bRet);
			return (command.status == 0);
		}
		return false;
	}

private:
	struct m_config {
		bool				bInitialize;
		bool				bRun;
		HANDLE				hShutdown;
		void				*pRunLoopPtr;
		PFUNC_AGENT_RUNLOOP	pRunLoopFunc;

		struct {
			WCHAR			szData[AGENT_PATH_SIZE];
			WCHAR			szDump[AGENT_PATH_SIZE];
			WCHAR			szApp[AGENT_PATH_SIZE];
			WCHAR			szDriver[AGENT_PATH_SIZE];			
		
		}	path;		
	} m_config;
	DbMap					m_db;
	CThread					m_main;

	bool			Initialize();
	void			Destroy(PCSTR pCause);
	void			AddDbList(int nResourceID, PCWSTR pRootPath, PCWSTR pName, DbMap & table);

	static	void	MainThread(void * ptr, HANDLE hShutdown);
	static	bool	CALLBACK	IPCRecvCallback(
		IN	HANDLE		hClient,
		IN	IIPCClient	*pClient,
		IN	void		*pContext,
		IN	HANDLE		hShutdown
	);
};