#include "pch.h"

bool	YAgent::SetFileContent(
	IN	LPCTSTR lpPath, IN PVOID lpContent, IN DWORD dwSize
)
{
	bool	bRet	= false;
	HANDLE	hFile;
	DWORD	dwWrittenBytes;

	//_log(_T("SET_FILECONTENT:%s[%d]"), szPath, dwSize);

	// 파일 생성시 옵션을 OPEN_ALWAYS 에서 CREATE_ALWAYS 로 변경함.
	// 이 함수의 주된 사용 목적이 디버깅/테스트를 위한 파일 생성이고 그러므로 
	// 기존 파일이 존재하면서 현재 쓰려는 양보다 더 크게 존재하는 경우 
	// 기존 파일의 내용이 뒤에 쓰레기 값으로 붙기 때문이다.
	// 2005.12.11 김범진

	// CreateFile 시에 적용된 옵션이 OPEN_ALWAYS로 되어 있었다.
	// 이 함수는 새로운 파일을 만드는 함수이므로 OPEN_ALWAYS -> CREATE_ALWAYS 로 변경되어야 맞을것.
	// 내부적으로 테스트 용도로만 사용된 함수이기에 이로 인한 버그는 여태 드러나지 않았을 것이다.

	::SetFileAttributes(lpPath, FILE_ATTRIBUTE_ARCHIVE);

	// 2013/11/04	kimpro
	// SetFileContent 이 호출되고 진행되는 가운데, 
	// 동일한 파일에 대해 다시 SetFileContent가 호출된다면 어떻게 될 것인가?
	// 아래 코드는 먼저 진행된 SetFileContent가 완료되지 않은 상태에서 
	// 이후 또 다른 SetFileContent가 수행될 수 있게 한다.
	// 왜?
	// 처음 CreateFile시 설정된 dwDesiredAccess 의 값이 두번째 CreateFile시 설정된 dwShareMode 와 같기때문에.
	// 참고 URL: http://http://kuaaan.tistory.com/407
	//

	TCHAR	szPath[LBUFSIZE]	= _T("");

	if( _T('\\') == lpPath[0] && _T('\\') != lpPath[1] )
	{
		// 현재 폴더를 기준으로 한 상대 경로
		GetModulePath(szPath, sizeof(szPath));
		::StringCbCat(szPath, sizeof(szPath), lpPath);
		lpPath	= szPath;
	}
#ifdef PERMIT_DUPLICATED_JOB

	hFile	= ::CreateFile(
		lpPath, 
		GENERIC_READ|GENERIC_WRITE, 
		FILE_SHARE_READ|FILE_SHARE_WRITE, 
		NULL, 
		CREATE_ALWAYS, 
		FILE_ATTRIBUTE_NORMAL|FILE_ATTRIBUTE_HIDDEN, 
		NULL);

#else

	hFile	= ::CreateFile(
		lpPath, 
		GENERIC_WRITE, 
		FILE_SHARE_READ,
		NULL, 
		CREATE_ALWAYS, 
		FILE_ATTRIBUTE_NORMAL|FILE_ATTRIBUTE_HIDDEN, 
		NULL);

#endif
	DWORD	dwError	= 0;
	if( INVALID_HANDLE_VALUE == hFile )
	{
		return false;
	}
	if( ::WriteFile(hFile, lpContent, dwSize, &dwWrittenBytes, NULL) && 
		dwWrittenBytes == dwSize )
	{
		bRet	= true;
	}
	else
	{
		dwError	= GetLastError();
	}
	::CloseHandle(hFile);
	if( bRet )
		::SetFileAttributes(szPath, FILE_ATTRIBUTE_NORMAL);
	SetLastError(dwError);
	return bRet;
}
PVOID	YAgent::GetFileContent(IN LPCTSTR lpPath, OUT DWORD * pSize)
{
	HANDLE		hFile		= INVALID_HANDLE_VALUE;
	char		*pContent	= NULL;
	DWORD		dwReadBytes	= 0;
	DWORD		dwSize		= 0;

	TCHAR	szPath[LBUFSIZE]	= _T("");

	if( _T('\\') == lpPath[0] && _T('\\') != lpPath[1] )
	{
		// 현재 폴더를 기준으로 한 상대 경로
		GetModulePath(szPath, sizeof(szPath));
		::StringCbCat(szPath, sizeof(szPath), lpPath);
		lpPath	= szPath;
	}
	__try
	{
		hFile	= ::CreateFile(lpPath, GENERIC_READ, FILE_SHARE_READ, NULL, 
			OPEN_EXISTING, FILE_ATTRIBUTE_ARCHIVE, NULL);
		if( INVALID_HANDLE_VALUE == hFile )
		{
			//_tprintf(_T("error: can not open file[%s], code = %d\n"), szPath, ::GetLastError());
			dwSize	= 0;
			__leave;
		}
		dwSize	= ::GetFileSize(hFile, NULL);
		if( 0 == dwSize )
		{
			__leave;
		}
		pContent	= (char *)CMemoryOperator::Allocate(dwSize + 2);
		if( NULL == pContent )
		{
			__leave;
		}
		if( (FALSE == ::ReadFile(hFile, pContent, dwSize, &dwReadBytes, NULL)) ||
			(dwReadBytes != dwSize) )
		{
			_tprintf(_T("error: ReadFile() failed.\n"));

			delete pContent;
			pContent	= NULL;
			__leave;
		}
		*(pContent + dwSize)		= NULL;
		*(pContent + dwSize + 1)	= NULL;
	}
	__finally
	{
		if( INVALID_HANDLE_VALUE != hFile )
		{
			::CloseHandle(hFile);
		}

		if( pSize )
		{
			*pSize	= dwSize;
		}
	}
	return pContent;
}
void	YAgent::FreeFileContent(PVOID p) {
	if( p )	CMemoryOperator::Free(p);
}