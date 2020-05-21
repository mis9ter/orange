// DelOldFolders.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include <iostream>
#include <Windows.h>
#include <tchar.h>
#include <strsafe.h>

#define     __MAX_PATH       1024


bool	GetInstancePath(IN HINSTANCE hInstance, OUT LPTSTR szPath, IN DWORD dwSize)
{
	LPTSTR	ps;
	TCHAR	szBuf[__MAX_PATH];

	if (::GetModuleFileName(hInstance, szBuf, sizeof(szBuf)))
	{
		ps = _tcsrchr(szBuf, TEXT('\\'));
		if (ps)
		{
			*ps = NULL;

			/*
				2013/05/05	kimpro
				도대체 아래 제일 처음이 역슬래시인 경우 왜 +4부터 복사했을까??
				==> \\??\\ .. 바보야..
			if( _T('\\') == szBuf[0] )
			{
				::StringCbCopy(szPath, dwSize, szBuf + 4);
			}
			else
			{
				::StringCbCopy(szPath, dwSize, szBuf);
			}
			*/
			::StringCbCopy(szPath, dwSize, szBuf);
			return true;
		}
	}
	return false;
}
bool	GetModulePath(OUT LPTSTR szPath, IN DWORD dwSize)
{
	return GetInstancePath(NULL, szPath, dwSize);
}

PCWSTR		FileTimeToLocalSystemTimeString(FILETIME* pFileTime, PWSTR pStr, DWORD dwStrSize)
{
	FILETIME	fTime;
	SYSTEMTIME	sTime;
	FileTimeToLocalFileTime(pFileTime, &fTime);
	FileTimeToSystemTime(&fTime, &sTime);
	StringCbPrintf(pStr, dwStrSize, L"%04d-%02d-%02d %02d:%02d:%02d", 
		sTime.wYear, sTime.wMonth, sTime.wDay, sTime.wHour, sTime.wMinute, sTime.wDay);
	return pStr;
}
bool		IsOldFile(IN FILETIME* pFileTime, IN FILETIME* pLastTime)
{
	LARGE_INTEGER	current, last;

	current.HighPart = pFileTime->dwHighDateTime;
	current.LowPart = pFileTime->dwLowDateTime;
	last.HighPart = pLastTime->dwHighDateTime;
	last.LowPart = pLastTime->dwLowDateTime;

	bool	bRet	= (current.QuadPart <= last.QuadPart) ? true : false;
	return bRet;
}
void		PrintDepth(IN size_t nDepth)
{
	for(size_t i = 0 ; i < nDepth ; i++)
		printf("    ");
}
bool      FindOldFiles
(
	IN size_t nDepth, IN PCWSTR pRootPath, 
	IN FILETIME * pTime, IN FILETIME * pLastTime,
	OUT	size_t * pFolders, OUT size_t * pFiles
)
{
	HANDLE		hFind	= INVALID_HANDLE_VALUE;
	wchar_t		szPath[__MAX_PATH];

	if( NULL == pLastTime || 0 == pLastTime->dwHighDateTime )	return false;

	StringCbPrintf(szPath, sizeof(szPath), L"%s\\*.*", pRootPath);

	WIN32_FIND_DATA	data;
	hFind = FindFirstFile(szPath, &data);
	bool	bIsThisOld	= false;

	auto	Show = [](size_t nDepth, PCWSTR pName, FILETIME* pTime, FILETIME * pLastTime, bool bCR)->bool
	{
		bool	bIsOld	= false;
		if (NULL == pTime)
		{
			printf("%ws\n", pName);
		}
		else
		{
			wchar_t		szTime[40];
			printf("\r");
			PrintDepth(nDepth);
			printf("[%c] %-30ws %ws%s",
				(bIsOld=IsOldFile(pTime, pLastTime)) ? 'o' : 'x',
				pName,
				FileTimeToLocalSystemTimeString(pTime, szTime, sizeof(szTime)),
				bCR? "\n":"");
		}
		return bIsOld;
	};
	auto	Error = [](size_t nDepth, PCWSTR pPath)
	{
		CHAR	szErrMsg[1024];
		DWORD	dwError = ::GetLastError();
		::FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwError, 0, szErrMsg, sizeof(szErrMsg), NULL);
		printf("\n");
		PrintDepth(nDepth);
		printf("error(%d) %ws : %s\n", dwError, pPath, szErrMsg);
	};
	bIsThisOld	= Show(nDepth, pRootPath, pTime, pLastTime, true);
	size_t		nFiles	= 0;
	if( INVALID_HANDLE_VALUE != hFind )
	{
		while( true )
		{
			if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if (L'.' != data.cFileName[0])
				{
					StringCbPrintf(szPath, sizeof(szPath), L"%s\\%s", pRootPath, data.cFileName);
					if (FindOldFiles(nDepth + 1, szPath, &data.ftLastWriteTime, pLastTime, pFolders, pFiles))
					{
						if (RemoveDirectory(szPath) )
						{							
							if( pFolders)	*pFolders++;
						}
						else
						{
							Error(nDepth + 1, szPath);
						}
					}

				}
			}
			else
			{
				nFiles++;
				if (Show(nDepth + 1, data.cFileName, &data.ftLastWriteTime, pLastTime, false))
				{
					StringCbPrintf(szPath, sizeof(szPath), L"%s\\%s", pRootPath, data.cFileName);
					SetFileAttributes(szPath, FILE_ATTRIBUTE_ARCHIVE);
					if( DeleteFile(szPath) )
					{
						nFiles--;
						if( pFiles )	*pFiles++;
						printf("  deleted.");
					}
					else
					{
						Error(nDepth + 1, szPath);
					}
				}
			}
			if( FALSE == FindNextFile(hFind, &data) )	break;
		}
		FindClose(hFind);
	}
	return (0 == nFiles)? true : bIsThisOld;
}

int main(int ac, wchar_t ** av)
{
	char	szDate[30]	= "";
	std::cout << "Delete old folder & files\n";
	printf("%-30s: ", "last DATE to delete:");
	std::cin >> szDate;

    wchar_t     szCurPath[_MAX_PATH]    = L"";
	GetCurrentDirectory(sizeof(szCurPath), szCurPath);
	printf("%-30s: %ws\n", "ROOT PATH to delete", szCurPath);

	SYSTEMTIME	sTime	= {0,};
	sTime.wYear		= atoi(szDate)/10000;
	sTime.wMonth	= (atoi(szDate)%10000)/100;
	sTime.wDay		= atoi(szDate)%100;

	printf("%-30s: %04d/%02d/%02d(YYYY/MM/DD)\n", "LAST DATE to delete", 
		sTime.wYear, sTime.wMonth, sTime.wDay);

	size_t		nFolders=0, nFiles=0;
	FILETIME	fTime[2];	
	SystemTimeToFileTime(&sTime, &fTime[0]);
	LocalFileTimeToFileTime(&fTime[0], &fTime[1]);
	FindOldFiles(0, szCurPath, NULL, &fTime[1], &nFolders, &nFiles);

	printf("\r\n%d folder(s), %d file(s) are deleted.\n", nFolders, nFiles);
}

// 프로그램 실행: <Ctrl+F5> 또는 [디버그] > [디버깅하지 않고 시작] 메뉴
// 프로그램 디버그: <F5> 키 또는 [디버그] > [디버깅 시작] 메뉴

// 시작을 위한 팁: 
//   1. [솔루션 탐색기] 창을 사용하여 파일을 추가/관리합니다.
//   2. [팀 탐색기] 창을 사용하여 소스 제어에 연결합니다.
//   3. [출력] 창을 사용하여 빌드 출력 및 기타 메시지를 확인합니다.
//   4. [오류 목록] 창을 사용하여 오류를 봅니다.
//   5. [프로젝트] > [새 항목 추가]로 이동하여 새 코드 파일을 만들거나, [프로젝트] > [기존 항목 추가]로 이동하여 기존 코드 파일을 프로젝트에 추가합니다.
//   6. 나중에 이 프로젝트를 다시 열려면 [파일] > [열기] > [프로젝트]로 이동하고 .sln 파일을 선택합니다.
