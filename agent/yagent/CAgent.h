#pragma once

#pragma warning(disable:4819)

#include <Windows.h>
#include <functional>
#include <thread>

#include "yagent.h"
#include "yagent.common.h"
#include "CThread.h"
#include "IFilterCtrl.h"
#include "CDialog.h"
#include "CException.h"
#include "CNotifyCenter.h"
#include "CIpc.h"
#include "CService.h"

#include "CEventCallback.h"

typedef std::function<void(HANDLE hShutdown, void * pCallbackPtr)>	PFUNC_AGENT_RUNLOOP;

class CFilePath
	:
	virtual	public	CAppLog
{
public:
	CFilePath() {

	}
	~CFilePath() {

	}
	bool	SetResourceToFile(UINT nId, LPCWSTR pFilePath)
	{
		CMemoryPtr	res = GetResourceData(NULL, nId);
		if (res && res->Data() && res->Size())
			if (SetFileData(pFilePath, res->Data(), res->Size())) {
				Log("%s %ws", __FUNCTION__, pFilePath);
				return true;
			}
			else {
				Log("%s %ws - SetFileData() failed.", __FUNCTION__, pFilePath);
			}	
		return false;
	}
	static	LPCTSTR		CheckBadBeomStyle(IN LPCTSTR pStr)
	{
		//	상대 경로를 표시하기 위해 예전에 주로 하던 짓이 
		//	현재 경로의 파일을 config.exe 로 쓰지 않고 \config.exe로 쓰는걸 즐겼다. 
		//	config\config2.cfg 도 역시 \config\config2.cfg 라고 종종 씀. 
		//	이렇게 앞에 \\가 붙어 있는 경로는 분명 상대경로이고 난 알아 먹겠는데
		//	윈도API들은 알아주질 않으니 이런 경로는 알아먹게 앞의 \빼준다.

		//	이 문자열이 최소 2자 이상일 때만 관리해줌. 
		if (NULL == pStr || NULL == *pStr || NULL == *(pStr + 1))	return pStr;
		//	두번째 것도 \이라면 '\\'와 같이 UNC 경로를 표시한 것이므로 건들지 말아요.
		if (_T('\\') == *pStr && _T('\\') != *(pStr + 1))	return pStr + 1;
		return pStr;
	}
	static	bool		ConvertPath(IN LPCTSTR pSrcPath,
		OUT LPTSTR pDestPath, IN DWORD dwDestSize)
	{
		/*
			상대 경로 => 절대 경로

			[TODO]
			그외 환경변수로 구성된 경로 => 절대 경로
		*/
		if (NULL == pSrcPath)	return false;

		pSrcPath = CheckBadBeomStyle(pSrcPath);
		if (::PathIsRelative(pSrcPath))
		{
			//	여기서 오만가지 경우를 다 처리해주겠다고 
			//	GetFullPathName / _wfullpath 함수를 사용했는데 익셉션 발생.  => 원인 파악 불가. 
			//	MSDN의 다음 문장을 보고 사용 포기.
			/*
				https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getfullpathnamea
				Multithreaded applications and shared library code should not use the GetFullPathName function and
				should avoid using relative path names.
				The current directory state written by the SetCurrentDirectory function is stored as a global variable in each process,
				therefore multithreaded applications cannot reliably use this value without possible data corruption
				from other threads that may also be reading or setting this value.

				This limitation also applies to the SetCurrentDirectory and GetCurrentDirectory functions.
				The exception being when the application is guaranteed to be running in a single thread,
				for example parsing file names from the command line argument string in the main thread
				prior to creating any additional threads.

				Using relative path names in multithreaded applications or shared library code can yield unpredictable results and
				is not supported.
			*/
			//	상대경로라면 절대 경로로 변경한다.
			TCHAR	szModulePath[AGENT_PATH_SIZE] = _T("");
			::GetModuleFileName(NULL, szModulePath, sizeof(szModulePath));
			/*
				Path 시리즈는 편하지만, 대신 MAX_PATH 라는 길이 제한이 있어 쓰기 싫은데..
			*/
			::PathRemoveFileSpec(szModulePath);
			::PathCombine(pDestPath, szModulePath, pSrcPath);
			return true;
		}
		::StringCbCopy(pDestPath, dwDestSize, pSrcPath);
		return true;
	}
	static	LPCTSTR		GetRelativePath(IN LPCTSTR pSrcPath)
	{
		//	절대 경로의 포인터를 받아서 현재 경로 대비 상대 경로의 시작점을 반환. 
		//	C:\\My Folder\\Iam.exe\\config\\config2.cfg => config\\config2.cfg
		pSrcPath = CheckBadBeomStyle(pSrcPath);
		if (::PathIsRelative(pSrcPath))
		{
			//	이미 생긴 꼴이 상대 경로라면 그대로 리턴
			return pSrcPath;
		}
		TCHAR		szCurPath[AGENT_PATH_SIZE];
		GetModuleFileName(NULL, szCurPath, sizeof(szCurPath));
		PathRemoveFileSpec(szCurPath);

		DWORD		dwSrcLen = lstrlen(pSrcPath);
		DWORD		dwCurLen = lstrlen(szCurPath);

		//	+1 은 슬래시, 받은 경로가 현재 내 실행경로보다 짧다면 상대경로인가? 
		//	모르겠으니 받은걸 그대로 리턴. 잘먹고 잘 살아라.
		return (dwSrcLen > dwCurLen) ? (pSrcPath + dwCurLen + 1) : pSrcPath;
	}
	static	CMemoryPtr	GetResourceData(IN HMODULE hModule, IN UINT nResourceId)
	{
		HRSRC		hRes = NULL;
		HGLOBAL		hData = NULL;
		CMemoryPtr	ptr;

		auto		GetModulePath = [&](IN HMODULE hModule, OUT LPTSTR pPath, IN DWORD dwSize)
		{
			GetModuleFileName(hModule, pPath, dwSize);
			return pPath;
		};

		TCHAR		szPath[AGENT_PATH_SIZE] = _T("");
		do
		{
			if (NULL == (hRes = ::FindResource(hModule, MAKEINTRESOURCE(nResourceId), RT_RCDATA)))
			{
				break;
			}
			if (NULL == (hData = ::LoadResource(hModule, hRes)))
			{
				break;
			}
			DWORD	dwSize = ::SizeofResource(hModule, hRes);
			LPVOID	pData = ::LockResource(hData);
			if (pData && dwSize)
			{
				if (ptr = CMemoryFactory::Instance()->AllocatePtr(__FUNCTION__, pData, dwSize))
				{

				}
				else
				{

				}
			}
			else
			{
#ifdef __log_config
				_nlogA(__log_config, "LockResource() failed. path=%s, code=%d",
					GetModulePath(hModule, szPath, sizeof(szPath)), ::GetLastError());
#endif
			}
		} while (false);
		/*
			hData, pData 이런 것들을 해제하지 않는 이유:
			이 프로세스의 내용이고 이미 메모리에 올라가 있어요.
			프로세스 종료시 알아서 사라집니다.
			걱정마세요.
		*/
		return ptr;
	}
	static	bool		SetFileData(IN LPCTSTR lpPath, IN LPCVOID pData, IN DWORD dwSize)
	{
		bool	bRet = false;
		DWORD	dwWrittenBytes;

		//	CREATE_ALWAYS
		//	기존 파일이 존재하면서 현재 쓰려는 양보다 더 크게 존재하는 경우 
		//	기존 파일의 내용이 뒤에 쓰레기 값으로 붙기 때문이다.
		//	FILE_SHARE_READ
		//	SetFileContent 작업이 끝나지 않은 상태에서 또 다시 호출되어 뒤죽박죽을 막기 위해
		//	Read만 공유.
		TCHAR	szPath[AGENT_PATH_SIZE] = _T("");
		HANDLE	hFile = INVALID_HANDLE_VALUE;

		ConvertPath(lpPath, szPath, sizeof(szPath));
		::SetFileAttributes(szPath, FILE_ATTRIBUTE_ARCHIVE);
		__try
		{
			hFile = ::CreateFile(
				szPath,
				GENERIC_WRITE,
				FILE_SHARE_READ,
				NULL,
				CREATE_ALWAYS,
				FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_HIDDEN,
				NULL);
			if (INVALID_HANDLE_VALUE == hFile)
			{
				__leave;
			}
			if (::WriteFile(hFile, pData, dwSize, &dwWrittenBytes, NULL) && dwWrittenBytes == dwSize)	bRet = true;
		}
		__finally
		{
			if (INVALID_HANDLE_VALUE != hFile)	::CloseHandle(hFile);
		}
		::SetFileAttributes(szPath, FILE_ATTRIBUTE_NORMAL);
		return bRet;
	}
	static	CMemoryPtr	GetFileData(IN LPCTSTR lpPath)
	{
		HANDLE		hFile = INVALID_HANDLE_VALUE;
		LPVOID		pData = NULL;
		DWORD		dwReadBytes = 0;
		DWORD		dwSize = 0;
		CMemoryPtr	ptr;

		TCHAR	szPath[AGENT_PATH_SIZE] = _T("");
		ConvertPath(lpPath, szPath, sizeof(szPath));
		do
		{
			hFile = ::CreateFile(szPath, GENERIC_READ, FILE_SHARE_READ, NULL,
				OPEN_EXISTING, FILE_ATTRIBUTE_ARCHIVE, NULL);
			if (INVALID_HANDLE_VALUE == hFile)
			{
				printf("CreateFile() failed. code=%d\n", ::GetLastError());
				dwSize = 0;
				break;
			}
			dwSize = ::GetFileSize(hFile, NULL);
			if (0 == dwSize)		break;
			ptr = CMemoryFactory::Instance()->AllocatePtr(__FUNCTION__, NULL, dwSize);
			if (nullptr == ptr)		break;
			if ((FALSE == ::ReadFile(hFile, ptr->Data(), ptr->Size(), &dwReadBytes, NULL)) || (dwReadBytes != ptr->Size()))
			{
				ptr = nullptr;
				break;
			}
		} while (false);
		if (INVALID_HANDLE_VALUE != hFile)
			::CloseHandle(hFile);
		return ptr;
	}
private:

};

class CAgent
	:
	public	CDB,
	public	CFilterCtrl,
	public	CEventCallback,
	public	CFilePath,
	public	CNotifyCenter,
	public	CIPc
{
public:
	CAgent();
	virtual	~CAgent();

	CDB* Db() {
		return dynamic_cast<CDB*>(this);
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
		return m_config.szAppPath;
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
private:
	struct m_config {
		bool				bInitialize;
		bool				bRun;
		HANDLE				hShutdown;
		void				*pRunLoopPtr;
		PFUNC_AGENT_RUNLOOP	pRunLoopFunc;

		WCHAR				szPath[AGENT_PATH_SIZE];
		WCHAR				szAppPath[AGENT_PATH_SIZE];
		WCHAR				szDriverPath[AGENT_PATH_SIZE];
		WCHAR				szEventDBPath[AGENT_PATH_SIZE];
	} m_config;
	CThread					m_main;

	bool			Initialize();
	void			Destroy();

	static	void	MainThread(void * ptr, HANDLE hShutdown);
	static	bool	CALLBACK	IPCCallback(
		IN	HANDLE		hClient,
		IN	IIPcClient	*pClient,
		IN	void		*pContext,
		IN	HANDLE		hShutdown
	);
};