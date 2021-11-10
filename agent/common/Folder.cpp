#include "pch.h"

YAGENT_COMMON_BEGIN
bool	GetAppTempPath(OUT LPTSTR szPath, IN DWORD dwSize)
{
	::GetTempPath(dwSize/sizeof(TCHAR), szPath);
	if( PathIsDirectory(szPath) )
	{
		return true;
	}
	if( ::SHGetSpecialFolderPath(NULL, szPath, CSIDL_APPDATA, TRUE) )
		return true;
	return false;
}
bool	DeleteDirectoryFiles(IN LPCTSTR lpPath)
{
	int					nErrorCount	= 0;
	WIN32_FIND_DATA		fData;
	TCHAR				szCurPath[MBUFSIZE]	= _T("");
	HANDLE				hFind	= INVALID_HANDLE_VALUE;
	TCHAR				szProgramFilesPath[MBUFSIZE]	= _T("");
	TCHAR				szWindowPath[MBUFSIZE]			= _T("");
	TCHAR				szSystemPath[MBUFSIZE]			= _T("");

	TCHAR				szPath[LBUFSIZE];
	if( _T('\\') == lpPath[0] && _T('\\') != lpPath[1] )
	{
		// 주어진 Path가 절대 경로가 아닌 상대 경로이다.
		GetModulePath(szPath, sizeof(szPath));
		::StringCbCat(szPath, sizeof(szPath), lpPath);

		lpPath	= szPath;
	}

	if( ! IsDirectory(lpPath) )
	{
		// 현재 존재하지 않는 디렉터리 이므로 삭제된 것과 동일하다고 본다.
		return true;
	}
	if( PathIsSystemFolder(lpPath, 0) || 3>= lstrlen(lpPath) )	return false;
	::GetCurrentDirectory(sizeof(szCurPath), szCurPath);
	if( !::SetCurrentDirectory(lpPath) )
	{
		// 2006.06.13	김범진,조인중
		// SetCurrentDirectory의 성공 여부와 상관없이 이후 작업을 진행 했던 기존 코드를
		// SetCurrentDirectory의 결과가 실패할때는 더 이상 진행하지 않고 리턴함.
		return false;
	}

	hFind	= ::FindFirstFile(_T("*.*"), &fData);
	if( INVALID_HANDLE_VALUE != hFind )
	{
		while( true )
		{
			if( fData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
			{
				if( lstrcmp(fData.cFileName, _T(".")) &&
					lstrcmp(fData.cFileName, _T("..")) )
				{
					TCHAR		szPath[LBUFSIZE];

					::StringCbPrintf(szPath, sizeof(szPath), _T("%s\\%s"), 
						lpPath, fData.cFileName);
					if( DeleteDirectoryFiles(szPath) )
					{
						if( ::RemoveDirectory(szPath) )
						{

						}
						else
						{
							nErrorCount++;
						}
					}
					else
					{
						nErrorCount++;
					}
				}
			}
			else
			{
				// File
				TCHAR		szPath[LBUFSIZE];

				::StringCbPrintf(szPath, sizeof(szPath), _T("%s\\%s"), 
					lpPath, fData.cFileName);
				::SetFileAttributes(szPath, FILE_ATTRIBUTE_ARCHIVE);
				if( ::DeleteFile(szPath) )
				{
					//	2006.06.13	김범진,조인중
					//	어차피 제일 밑의 RemoveDirectory에서 실패하면 실패로 처리
					//	되니 여기서 bRet의 값을 설정하는 삽질은 안하기로 하자.
					//bRet	= true;
					//_tprintf(_T("delete %s.\n"), strPath.c_str());
				}
				else
				{
					nErrorCount++;
				}
			}

			if( FALSE == ::FindNextFile(hFind, &fData) )
			{
				break;
			}
		}
		::FindClose(hFind);
	}
	::SetCurrentDirectory(szCurPath);

	return nErrorCount? false : true;
}

bool	DeleteDirectory(IN LPCTSTR lpPath)
{
	// 아래 함수와의 차이점은 디렉토리는 지우지 않고 내부의 파일만 지운다는 것. 
	::SetFileAttributes(lpPath, FILE_ATTRIBUTE_NORMAL);
	if( DeleteDirectoryFiles(lpPath) )
	{
		if( ::RemoveDirectory(lpPath) )
		{
			return true;
		}
	}
	return false;
}
YAGENT_COMMON_END