#include "pch.h"
#include <shlobj.h>

YAGENT_COMMON_BEGIN

bool	GetLinkPath(IN LPCTSTR pLinkFile, 
					OUT LPTSTR pLinkPath, IN DWORD dwLinkPathSize,
					OUT WIN32_FIND_DATA *pFd)
{
	bool		bRet	= false;
	IShellLink	*pLink;
	LPCTSTR		pExt;
	WIN32_FIND_DATA	fd;

	//_log(_T("GET_LINK_PATH:%s"), pLinkFile);

	if( NULL == pFd )
	{
		pFd	= &fd;
	}

	pExt	= GetFileExt(pLinkFile);

	if( NULL == pExt || _tcsicmp(pExt, _T("lnk")) )
	{
		//_log(_T("error: pExt[%08x] is NULL or pExt[%s] is not \"lnk\""), pExt, pExt);
		return false;
	}

	//_log(_T("__begin_get_link_path:%s"), pLinkFile);

	*pLinkPath	= NULL;

	HRESULT	hr	= ::CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
										IID_IShellLink, (LPVOID *)&pLink);

	if( SUCCEEDED(hr) )
	{
		IPersistFile	*pFile;

		hr	= pLink->QueryInterface(IID_IPersistFile, (void **)&pFile);
		if( SUCCEEDED(hr) )
		{
			if( SUCCEEDED(pFile->Load(pLinkFile, STGM_READ)) )
			{	
				if( SUCCEEDED(pLink->Resolve(NULL, SLR_NO_UI|SLR_NOUPDATE)) )
				{					
					if( SUCCEEDED(pLink->GetPath(pLinkPath, dwLinkPathSize/sizeof(TCHAR), 
							(WIN32_FIND_DATA *)pFd, 0)) )
					{
						bRet	= true;
					}
					else
					{
						//_log(_T("error: pLink->GetPath() failed. path = [%s], code = %d"), pLinkFile, ::GetLastError());
					}
				}
				else
				{
					//_log(_T("error: pLink->Resolve() failed. path = [%s], code = %d"), pLinkFile, ::GetLastError());
				}				
			}
			else
			{
				//_log(_T("error: pFile->Load() failed. path = %s, code = %d"), pLinkFile, ::GetLastError());
			}
			pFile->Release();
		}
		else
		{
			//_log(_T("error: pLink->QueryInterface() failed. path = %s, code = %d"), pLinkFile, ::GetLastError());
		}
		pLink->Release();
	}
	else
	{
		//_log(_T("error: CoCreateInstance() failed. hr = %08x, code = %d"), hr, ::GetLastError());
	}

	if( _T('\\') == *pLinkPath )
	{
		// UNC Folder
		// \\SERVER\D\WEB
		//
		TCHAR	szPath[LBUFSIZE];
		bool	bFindFirst	= false;
		TCHAR	cDrive		= 0;

		if( _T('\\') == *(pLinkPath+1) )
		{
			LPTSTR	pStr	= _tcschr(pLinkPath+2, _T('\\'));

			if( pStr && lstrlen(pStr) > 2 )
			{
				if( _toupper(*(pStr + 1)) >= _T('C') && _toupper(*(pStr + 1)) <= _T('Z') && *(pStr + 2) == _T('\\') )
				{
					::StringCbPrintf(szPath, sizeof(szPath), _T("%c:%s"), *(pStr + 1), pStr + 2); 
					::StringCbCopy(pLinkPath, dwLinkPathSize, szPath);

					bRet	= true;
				}
			}
		}
	}

	int	nLen	= lstrlen(pLinkPath);

	if( nLen && pLinkPath[nLen-1] == _T('\\') )
	{
		pLinkPath[nLen - 1] = NULL;
	}

	//_log(_T("__end_get_link_path:%s(%d)"), pLinkFile, bRet);

	return bRet;
}

bool	GetLinkInfo
(
	IN	LPCTSTR		pLinkFile, 
	OUT LPTSTR		pLinkPath, 
	IN	DWORD		dwLinkPathSize,
	OUT WIN32_FIND_DATA *pFd,
	OUT	LPTSTR		pArgument,
	IN	DWORD		dwArgumentSize,
	OUT	LPTSTR		pWorkingDirectory,
	IN	DWORD		dwWorkingDirectorySize,
	OUT	LPTSTR		pDescription,
	IN	DWORD		dwDescriptionSize
)
{
	bool		bRet	= false;
	IShellLink	*pLink;
	LPCTSTR		pExt;
	WIN32_FIND_DATA	fd;

	//_nlog(_T("xcommon"), _T("GET_LINK_PATH:%s"), pLinkFile);

	if( NULL == pFd )
	{
		pFd	= &fd;
	}

	pExt	= GetFileExt(pLinkFile);

	if( NULL == pExt || _tcsicmp(pExt, _T("lnk")) )
	{
		//_nlog(_T("xcommon"), _T("error: pExt[%08x] is NULL or pExt[%s] is not \"lnk\""), pExt, pExt);
		return false;
	}

	//_log(_T("__begin_get_link_path:%s"), pLinkFile);

	*pLinkPath	= NULL;

	HRESULT	hr	= ::CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
										IID_IShellLink, (LPVOID *)&pLink);

	if( SUCCEEDED(hr) )
	{
		IPersistFile	*pFile;

		hr	= pLink->QueryInterface(IID_IPersistFile, (void **)&pFile);
		if( SUCCEEDED(hr) )
		{
			if( SUCCEEDED(pFile->Load(pLinkFile, STGM_READ)) )
			{	
				if( SUCCEEDED(pLink->Resolve(NULL, SLR_NO_UI|SLR_NOUPDATE)) )
				{
					/*
						2015/12/24	KimPro

						GetLink로 구한 경로의 값이 틀릴 경우가 발생되었다.
						대상앱이 64bit인 경우 32bit에서 읽어들일 경우 실제 파일 경로는 C:\Program Files\... 인데
						C:\Program Files (x86)\... 로 얻어지는 경우이다.

						아마도 짐작컨데 경로 설정시 %ProgramFiles%로 넣었고 이를 32bit/64bit에 따라 서로 다른값으로 변환하기 
						때문이 아닐까?

						눈물을 머금고 경로에 "Program Files (x86)"이 존재하면서 해당 파일이 없다면 "Program Files"로 바꾸고
						다시 살펴보도록 한다.
					*/

					if( SUCCEEDED(pLink->GetPath(pLinkPath, dwLinkPathSize/sizeof(TCHAR), 
							(WIN32_FIND_DATA *)pFd, 0)) )
					{
						if( IsFileExist(pLinkPath) )
						{
							//_nlog(_T("xcommon"), _T("PATH:%s"), pLinkPath);
							bRet	= true;
						}
						else
						{
							//	파일이 존재하지 않고 프파x86이라면?
							if( _tcsstr(pLinkPath, _T(" (x86)")) )
							{
								ReplaceString(pLinkPath, dwLinkPathSize, _T("Program Files (x86)"), _T("Program Files"));
								if( IsFileExist(pLinkPath) )
								{
									//_nlog(_T("xcommon"), _T("PATH:%s is modified."), pLinkPath);
									bRet	= true;
								}
							}
						}
					}
					else
					{
						//_log(_T("error: pLink->GetPath() failed. path = [%s], code = %d"), pLinkFile, ::GetLastError());
					}
					if( pWorkingDirectory && SUCCEEDED(pLink->GetWorkingDirectory(pWorkingDirectory, dwWorkingDirectorySize/sizeof(TCHAR))) )
					{
						//_nlog(_T("xcommon"), _T("WORKINGPATH:%s"), pLinkPath);
						bRet	= true;
					}
					else
					{
						bRet	= false;
					}

					if( pArgument && SUCCEEDED(pLink->GetArguments(pArgument, dwArgumentSize/sizeof(TCHAR))) )
					{
						bRet	= true;
					}
					else
					{
						bRet	= false;
					}

					if( pDescription && SUCCEEDED(pLink->GetDescription(pDescription, dwDescriptionSize/sizeof(TCHAR))) )
					{
						bRet	= true;
					}
					else
					{
						bRet	= false;
					}

				}
				else
				{
					//_nlog(_T("xcommon"), _T("error: pLink->Resolve() failed. path = [%s], code = %d"), pLinkFile, ::GetLastError());
				}				
			}
			else
			{
				//_nlog(_T("xcommon"), _T("error: pFile->Load() failed. path = %s, code = %d"), pLinkFile, ::GetLastError());
			}
			pFile->Release();
		}
		else
		{
			//_nlog(_T("xcommon"), _T("error: pLink->QueryInterface() failed. path = %s, code = %d"), pLinkFile, ::GetLastError());
		}
		pLink->Release();
	}
	else
	{
		//_nlog(_T("xcommon"), _T("error: CoCreateInstance() failed. hr = %08x, code = %d"), hr, ::GetLastError());
	}

	if( _T('\\') == *pLinkPath )
	{
		// UNC Folder
		// \\SERVER\D\WEB
		//
		TCHAR	szPath[LBUFSIZE];
		bool	bFindFirst	= false;
		TCHAR	cDrive		= 0;

		if( _T('\\') == *(pLinkPath+1) )
		{
			LPTSTR	pStr	= _tcschr(pLinkPath+2, _T('\\'));

			if( pStr && lstrlen(pStr) > 2 )
			{
				if( _toupper(*(pStr + 1)) >= _T('C') && _toupper(*(pStr + 1)) <= _T('Z') && *(pStr + 2) == _T('\\') )
				{
					::StringCbPrintf(szPath, sizeof(szPath), _T("%c:%s"), *(pStr + 1), pStr + 2); 
					::StringCbCopy(pLinkPath, dwLinkPathSize, szPath);

					bRet	= true;
				}
			}
		}
	}

	int	nLen	= lstrlen(pLinkPath);

	if( nLen && pLinkPath[nLen-1] == _T('\\') )
	{
		pLinkPath[nLen - 1] = NULL;
	}

	//_log(_T("__end_get_link_path:%s(%d)"), pLinkFile, bRet);


	return bRet;
}

YAGENT_COMMON_END