#include "pch.h"

YAGENT_COMMON_BEGIN

DWORD64	GetFileSize64(IN HANDLE hFile)
{
	DWORD	dwHi, dwLo;
	DWORD64	dwFileSize	= 0, dwSize;

	dwLo	= ::GetFileSize(hFile, &dwHi);
	dwSize	= dwHi; 

	dwFileSize	= (dwSize<<32)|dwLo;

	return dwFileSize;
}

DWORD64	GetFileSize64ByPath(IN LPCTSTR lpPath)
{
	DWORD64	dwSize	= 0;
	HANDLE	hFile;

	hFile	= ::CreateFile(lpPath, GENERIC_READ, FILE_SHARE_READ, NULL, 
					OPEN_EXISTING, FILE_ATTRIBUTE_ARCHIVE, NULL);
	if( INVALID_HANDLE_VALUE == hFile )
	{
		// 그런 파일은 없어요.
		return dwSize;			
	}

	dwSize	= GetFileSize64(hFile);
	::CloseHandle(hFile);

	return dwSize;
}

bool	IsWindows9x()
{
	return false;
}
bool	FindFile(IN LPCTSTR lpPath, OUT WIN32_FIND_DATA *fd)
{
	WIN32_FIND_DATA		fData;
	//HANDLE				hFile;	-> 밑의 주석을 보시라.
	HANDLE				hFind;
	LPCTSTR				pPath;
	TCHAR				szShortPath[MBUFSIZE];

	if( IsWindows9x() && ::GetShortPathName(lpPath, szShortPath, sizeof(szShortPath)) )
	{
		pPath	= szShortPath;
	}
	else
	{
		pPath	= lpPath;
	}

	hFind	= ::FindFirstFile(pPath, &fData);
	if( INVALID_HANDLE_VALUE == hFind )
	{
		return false;
	}

	if( fd )
	{
		::CopyMemory(fd, &fData, sizeof(fData));
	}
	::FindClose(hFind);
	
	return true;
}


bool	IsFileExist(IN LPCTSTR lpPath)
{
	WIN32_FIND_DATA		fData;
	HANDLE				hFind;
	LPCTSTR				pPath;
	TCHAR				szPath[LBUFSIZE]		= _T("");

	// 2012/02/03	김프로
	// lpPath가 풀패스가 아닌 단순 파일명이라면?
	// 그렇다면 현재 디렉토리에서 해당 파일이 존재하는지 확인한다.

	if( lpPath && *lpPath && 
		(	
			lpPath[1] == _T(':') ||
			(lpPath[0] == _T('\\') && lpPath[1] == _T('\\'))
			
		)
	)
	{
		// 풀패스로 구성되어 있다.
		::StringCbCopy(szPath, sizeof(szPath), lpPath);
	}
	else
	{
		// 단순 파일명 - 현재 디렉토리에서 찾는다.
		LPTSTR	pStr	= NULL;

		::GetModuleFileName(NULL, szPath, sizeof(szPath));
		pStr	= _tcsrchr(szPath, _T('\\'));
		if( pStr )
		{
			*(pStr + 1)	= NULL;
			::StringCbCat(szPath, sizeof(szPath), lpPath);
		}
	}

	pPath	= szPath;

	hFind	= ::FindFirstFile(pPath, &fData);
	if( INVALID_HANDLE_VALUE == hFind )
	{
		//_log(_T("error: NOT_FOUND[%s], code = %d"), pPath, ::GetLastError());
		return false;
	}

	::FindClose(hFind);

	return true;
}

DWORD	GetFileSize32(IN LPCTSTR lpPath, OUT bool * bExist)
{
	WIN32_FIND_DATA		fData;
	//HANDLE				hFile;	-> 밑의 주석을 보시라.
	HANDLE				hFind;
	DWORD				dwSize;
	LPCTSTR				pPath;
	TCHAR				szShortPath[MBUFSIZE];

	if( IsWindows9x() && ::GetShortPathName(lpPath, szShortPath, sizeof(szShortPath)) )
	{
		pPath	= szShortPath;
	}
	else
	{
		pPath	= lpPath;
	}

	hFind	= ::FindFirstFile(pPath, &fData);
	if( INVALID_HANDLE_VALUE == hFind )
	{
		if( bExist )
		{
			*bExist	= false;
		}
		return 0;
	}
	dwSize	= fData.nFileSizeLow;
	::FindClose(hFind);

	/*
		2006.06.15	범진/인중
		위에서 이미 파일 크기를 다 얻을 수 있기 때문에 위에서 
		dwSize	= fData.nFileSizeLow;
		를 추가하고 (기존에는 없었음.) 아래 부분은 주석처리 하련다.
		
	// FILE_READ_ATTRIBUTE 가 속도면에서 더 유리할 것으로 생각했으나, 
	// WIN9X에서는 지원되지 않는다.
	hFile	= ::CreateFile(pPath, GENERIC_READ, //FILE_READ_ATTRIBUTES,
					FILE_SHARE_READ, NULL, OPEN_EXISTING, 
					FILE_ATTRIBUTE_NORMAL|FILE_ATTRIBUTE_SYSTEM|FILE_ATTRIBUTE_READONLY,
					NULL);
	if( INVALID_HANDLE_VALUE == hFile )
	{
		// FindFirstFile로는 있는데, CreateFile로는 없다? 오호 이상한 경우이어라.
		//AhnAlert(_T("%s\n%d"), pPath, GetLastError());
		*bExist	= false;
		return 0;
	}
	dwSize	= ::GetFileSize(hFile, NULL);
	::CloseHandle(hFile);
	*/

	if( bExist )
	{
		*bExist	= true;
	}
	return dwSize;
}

DWORD	GetFileSize(IN LPCTSTR lpPath, OUT bool * bExist)
{
	return GetFileSize32(lpPath, bExist);
}

YAGENT_COMMON_END