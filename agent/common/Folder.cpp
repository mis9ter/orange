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
		// �־��� Path�� ���� ��ΰ� �ƴ� ��� ����̴�.
		GetModulePath(szPath, sizeof(szPath));
		::StringCbCat(szPath, sizeof(szPath), lpPath);

		lpPath	= szPath;
	}

	if( ! IsDirectory(lpPath) )
	{
		// ���� �������� �ʴ� ���͸� �̹Ƿ� ������ �Ͱ� �����ϴٰ� ����.
		return true;
	}
	if( PathIsSystemFolder(lpPath, 0) || 3>= lstrlen(lpPath) )	return false;
	::GetCurrentDirectory(sizeof(szCurPath), szCurPath);
	if( !::SetCurrentDirectory(lpPath) )
	{
		// 2006.06.13	�����,������
		// SetCurrentDirectory�� ���� ���ο� ������� ���� �۾��� ���� �ߴ� ���� �ڵ带
		// SetCurrentDirectory�� ����� �����Ҷ��� �� �̻� �������� �ʰ� ������.
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
					//	2006.06.13	�����,������
					//	������ ���� ���� RemoveDirectory���� �����ϸ� ���з� ó��
					//	�Ǵ� ���⼭ bRet�� ���� �����ϴ� ������ ���ϱ�� ����.
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
	// �Ʒ� �Լ����� �������� ���丮�� ������ �ʰ� ������ ���ϸ� ����ٴ� ��. 
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