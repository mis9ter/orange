#include "pch.h"

bool	YAgent::SetFileContent(
	IN	LPCTSTR lpPath, IN PVOID lpContent, IN DWORD dwSize
)
{
	bool	bRet	= false;
	HANDLE	hFile;
	DWORD	dwWrittenBytes;

	//_log(_T("SET_FILECONTENT:%s[%d]"), szPath, dwSize);

	// ���� ������ �ɼ��� OPEN_ALWAYS ���� CREATE_ALWAYS �� ������.
	// �� �Լ��� �ֵ� ��� ������ �����/�׽�Ʈ�� ���� ���� �����̰� �׷��Ƿ� 
	// ���� ������ �����ϸ鼭 ���� ������ �纸�� �� ũ�� �����ϴ� ��� 
	// ���� ������ ������ �ڿ� ������ ������ �ٱ� �����̴�.
	// 2005.12.11 �����

	// CreateFile �ÿ� ����� �ɼ��� OPEN_ALWAYS�� �Ǿ� �־���.
	// �� �Լ��� ���ο� ������ ����� �Լ��̹Ƿ� OPEN_ALWAYS -> CREATE_ALWAYS �� ����Ǿ�� ������.
	// ���������� �׽�Ʈ �뵵�θ� ���� �Լ��̱⿡ �̷� ���� ���״� ���� �巯���� �ʾ��� ���̴�.

	::SetFileAttributes(lpPath, FILE_ATTRIBUTE_ARCHIVE);

	// 2013/11/04	kimpro
	// SetFileContent �� ȣ��ǰ� ����Ǵ� ���, 
	// ������ ���Ͽ� ���� �ٽ� SetFileContent�� ȣ��ȴٸ� ��� �� ���ΰ�?
	// �Ʒ� �ڵ�� ���� ����� SetFileContent�� �Ϸ���� ���� ���¿��� 
	// ���� �� �ٸ� SetFileContent�� ����� �� �ְ� �Ѵ�.
	// ��?
	// ó�� CreateFile�� ������ dwDesiredAccess �� ���� �ι�° CreateFile�� ������ dwShareMode �� ���⶧����.
	// ���� URL: http://http://kuaaan.tistory.com/407
	//

	TCHAR	szPath[LBUFSIZE]	= _T("");

	if( _T('\\') == lpPath[0] && _T('\\') != lpPath[1] )
	{
		// ���� ������ �������� �� ��� ���
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
		// ���� ������ �������� �� ��� ���
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