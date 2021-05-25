#include "pch.h"

bool	YAgent::Alert(PCWSTR fmt, ... )
{
	va_list	argptr;
	TCHAR	szBuf[LBUFSIZE];
	DWORD	dwCnt;
	TCHAR	szCaption[MBUFSIZE];

	if( NULL == fmt )
	{
		return false;
	}

	//	2017/07/27	BEOM
	//	�Ʒ� ������ ���� ���̵�� �˻��ϴ°ɷ� ��ü�Ѵ�.
	if( 0 == GetCurrentSessionId() )
		return 0;

	// 2014/02/17 ���� ����
	// �� ������� ���� �α� ���̵� "SYSTEM"�� ��� ���� ���� ���� ���̰ų� ���� ��忡 ���� ����� ����.
	// �� ��쿡�� ����ڿ��� UI�� ������� �ʰ�, �α� ���Ϸ� ������ ����Ѵ�.

	::GetModuleFileName(NULL, szCaption, sizeof(szCaption));
	va_start(argptr, fmt);
	dwCnt	= ::StringCbVPrintf(szBuf, sizeof(szBuf), fmt, argptr);

	{
		TCHAR	szUserId[SBUFSIZE]	= _T("");
		DWORD	dwUserIdSize		= sizeof(szUserId)/sizeof(TCHAR);

		::GetUserName(szUserId, &dwUserIdSize);
		if( !_tcsicmp(szUserId, _T("SYSTEM")) )
		{
			// ���� ���� ���� ��
			//_log(_T("ALERT:%s"), szBuf);
		}
		else
		{
			MessageBox(NULL, szBuf, szCaption, MB_OK|MB_ICONINFORMATION|MB_TOPMOST);
		}
	}
	va_end(argptr);
	/*
	2006.06.13	��&��
	return ���� ����� ������ ������ �Ϸ��� �߳� ���� �׷��� ����. ��������
	StringcbPrintf�� ������ ���̸� �������� �ʴ´�. �뼼�� ���� ��� �׳� ����.
	*/
	return (dwCnt / sizeof(TCHAR))? true : false;


}
