#include "framework.h"

LPCSTR	SystemTime2String(IN SYSTEMTIME* p, OUT LPSTR pStr, IN DWORD dwSize,
	IN bool bTimer)
{
	if (bTimer)
		::StringCbPrintfA(pStr, dwSize, "           %02d:%02d:%02d.%03d",
			p->wHour, p->wMinute, p->wSecond, p->wMilliseconds);
	else
		::StringCbPrintfA(pStr, dwSize, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
			p->wYear, p->wMonth, p->wDay, p->wHour, p->wMinute, p->wSecond, p->wMilliseconds);
	return pStr;
}

LPCSTR	UUID2StringA(IN UUID* p, OUT LPSTR pStr, IN DWORD dwSize)
{
	LPSTR	pBuf = NULL;
	if (RPC_S_OK == UuidToStringA(p, (RPC_CSTR*)&pBuf))
	{
		StringCbCopyA(pStr, dwSize, pBuf);
		RpcStringFreeA((RPC_CSTR*)&pBuf);
	}
	else 
		StringCbCopyA(pStr, dwSize, "");
	return pStr;
}
LPCWSTR	UUID2StringW(IN UUID* p, OUT LPWSTR pStr, IN DWORD dwSize)
{
	LPWSTR	pBuf = NULL;
	if (RPC_S_OK == UuidToStringW(p, (RPC_WSTR*)&pBuf))
	{
		StringCbCopyW(pStr, dwSize, pBuf);
		RpcStringFreeW((RPC_WSTR*)&pBuf);
	}
	else
	{
		StringCbCopy(pStr, dwSize, L"");
	}
	return pStr;
}