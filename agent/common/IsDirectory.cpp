#include "pch.h"
#include <shlwapi.h>

YAGENT_COMMON_BEGIN
bool	IsDirectory(LPCTSTR lpPath)
{
	WIN32_FIND_DATA		fd;

	///////////////////////////////////////////////////////////////////////////
	// 드라이브만 넘어온 경우
	///////////////////////////////////////////////////////////////////////////
	if( lpPath && lstrlen(lpPath) <= 3 )
	{
		TCHAR	szDrive[]	= _T("C:");
		szDrive[0]	= *lpPath;

		UINT	uRet	= ::GetDriveType(szDrive);

		if( DRIVE_UNKNOWN == uRet || DRIVE_NO_ROOT_DIR == uRet )
		{
			//Alert(_T("[%s] - DRIVE_UNKNOWN or DRIVE_NO_RROT_DIR"), szDrive);
			return FALSE;
		}
		return TRUE;
	}

	TCHAR	szPath[LBUFSIZE];
	TCHAR	szLastName[MBUFSIZE];
	LPCTSTR	pStr, pExt;
	bool	bLink	= false;

	pStr	= _tcsrchr(lpPath, _T('\\'));
	if( pStr )
	{
		::StringCbCopy(szLastName, sizeof(szLastName), pStr + 1);
	}
	else
	{
		::StringCbCopy(szLastName, sizeof(szLastName), lpPath);
	}

	pExt	= _tcsrchr(szLastName, _T('.'));
	if( pExt && !_tcsicmp(pExt, _T(".lnk")) )
	{
		// Link File
		if( GetLinkPath(lpPath, szPath, sizeof(szPath), NULL) )
		{
			if( PathIsURL(szPath) )
			{
				//Alert(_T("URL"));
				return FALSE;
			}
			lpPath	= szPath;
			
		}
	}

	HANDLE	hFile	= ::CreateFile(lpPath, FILE_READ_ATTRIBUTES, FILE_SHARE_READ, NULL, 
						OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);

	//_log(_T("[%s]hFile:%08x"), lpPath, hFile);
	if( INVALID_HANDLE_VALUE != hFile )
	{
		::CloseHandle(hFile);
	}

	//_log(_T("PathFileExists:%d"), ::PathFileExists(lpPath));


	DWORD	dwAttributes	= ::GetFileAttributes(lpPath);
	//_log(_T("[%s]: attributes=%08x"), lpPath, dwAttributes);
	if( INVALID_FILE_ATTRIBUTES == dwAttributes )
	{
		//Alert(_T("%s\nINVALID_FILE_ATTRIBUTES"), lpPath);
		return FALSE;
	}

	if( dwAttributes & FILE_ATTRIBUTE_DIRECTORY || 
		dwAttributes == 22 )
	{
		//Alert(_T("FILE_ATTRIBUTE_DIRECTORY"));
		return TRUE;
	}

	//Alert(_T("END"));

	return FALSE;

	hFile	= ::FindFirstFile(lpPath, &fd);
	if( INVALID_HANDLE_VALUE == hFile )
	{
		//_log(_T("error(%S): can not open file [%s], code = %d"), __FUNCTION__, lpPath, ::GetLastError());
		return FALSE;
	}
	FindClose(hFile);

	//_log(_T("attribute: %08x"), fd.dwFileAttributes);

	if(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY || fd.dwFileAttributes == 22 )
	{
		return TRUE;
	}
	return FALSE;
}
YAGENT_COMMON_END