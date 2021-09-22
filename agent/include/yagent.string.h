#pragma once

#include <Windows.h>
#include <strsafe.h>
#include <tchar.h>
#include <string>
#include <atlstr.h>
#include <map>

#define	STRING_BUFFER_SIZE	4096
#define	__ansi(str)			CANSI(str).Get()
#define __wide(str)			CWIDE(str).Get()
#define __utf8(str)			CUTF8(str).Get()
#define __utf16(str)		CUTF16(str).Get()
#define	__mstricmp(a,b)		_mbsicmp((const unsigned char*)(a? a:""), (const unsigned char*)(b? b:""))
#define	__mstrstr(a,b)		_mbsstr((const unsigned char*)(a? a:""), (const unsigned char*)(b? b:""))
#define	__wstricmp(a,b)		_wcsicmp((PCWSTR)(a? a:L""), (PCWSTR)(b? b:L""))

#define _t(p)				_T(p)

class CBuffer
{
public:
	CBuffer(IN DWORD dwSize = STRING_BUFFER_SIZE)
		:
		m_pBuf(NULL),
		m_dwSize(0)
	{
		m_pBuf		= HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize + 1);
		if( m_pBuf ) {
			m_dwSize = dwSize;
			((char *)m_pBuf)[dwSize]	= NULL;
		}
	}
	~CBuffer()
	{
		if (m_pBuf)	{
			HeapFree(GetProcessHeap(), 0, m_pBuf);
			m_pBuf	= NULL;
		}
	}
	void *		Data()
	{
		return  m_pBuf;
	}
	DWORD		Size()
	{
		return m_dwSize;
	}
private:
	void		*m_pBuf;
	DWORD		m_dwSize;
};
class CoPWSTR
{
public:
	CoPWSTR() 
		:
		m_pStr(NULL)
	{

	}
	~CoPWSTR() {
		if( m_pStr )	CoTaskMemFree(m_pStr);
	}
	operator PCWSTR() {
		return m_pStr;
	}
	operator PWSTR() {
		return m_pStr;
	}
	operator PWSTR *() {
		return &m_pStr;
	}
private:
	PWSTR		m_pStr;
};
class CWSTR
{
public:
	CWSTR(IN PCWSTR p)
		:
		CWSTR()
	{
		if( p )
		{
			m_str	= p;
		}
	}
	CWSTR()
	{
		m_szNull[0] = NULL;
	}
	~CWSTR()
	{

	}
	std::wstring &	operator =(CWSTR & p) {
		UNREFERENCED_PARAMETER(p);
		return m_str;
	}

	CWSTR &	operator =(std::wstring & p) {
		m_str	= p;		
		return *this;
	}
	CWSTR &	operator =(PWSTR & p) {
		m_str	= p;		
		return *this;
	}
	CWSTR &	operator =(PCWSTR & p) {
	
		m_str	= p;		
		return *this;
	}
	operator LPCWSTR()
	{
		return Get();
	}
	void		Set(PCWSTR p) {
		m_str	= p;
	}
	PCWSTR		Get()
	{
		return m_str.empty()? m_szNull : m_str.c_str();
	}
	size_t		CbSize()
	{
		return m_str.length() * sizeof(WCHAR);
	}
	size_t		Len() 
	{
		return m_str.length();
	}
private:
	std::wstring	m_str;
	WCHAR			m_szNull[1];
};

class CANSI
{
public:
	CANSI(IN LPCSTR  pStr)
	{
		m_szStr[0] = NULL;
		::StringCbCopyA(m_szStr, sizeof(m_szStr), pStr ? pStr : "");
		m_pStr = m_szStr;
	}
	CANSI(IN LPCTSTR pwStr)
	{
		m_szStr[0] = NULL;
		if (pwStr)
		{
			::WideCharToMultiByte(CP_ACP, 0, pwStr, -1, m_szStr, sizeof(m_szStr), NULL, NULL);
		}
		m_pStr = m_szStr;
	}
	CANSI(IN std::wstring str) : CANSI(str.c_str())
	{

	}
	~CANSI()
	{

	}
	LPCSTR	Get()
	{
		return m_pStr;
	}

private:
	char			m_szStr[STRING_BUFFER_SIZE];
	const	char* m_pStr;
};
class CWIDE
{
public:
	CWIDE(IN LPCSTR pStr)
		: m_pwStr(pStr ? AnsiToWide(pStr) : NULL)
	{
		m_dwLen = pStr ? lstrlen(m_pwStr) : 0;
	}
	CWIDE(IN std::string str)
		: CWIDE(str.c_str())
	{

	}
	~CWIDE()
	{
		if (m_pwStr)
		{
			FreeString((LPVOID)m_pwStr);
		}
	}
	LPCWSTR	Get()
	{
		static	WCHAR	szNull[] = _T("");

		return m_pwStr ? m_pwStr : szNull;
	}
	DWORD	Len()
	{
		return m_dwLen;
	}
	static	LPTSTR	AnsiToWide(LPCSTR szSrcStr, UINT uCodePage = CP_ACP)
	{
		if (NULL == szSrcStr)	return NULL;

		LPWSTR	pszDestStr = NULL;
		INT		nLength = 0;
		INT		nSize = 0;

		// Get ANSI Size
		nLength = ::MultiByteToWideChar(uCodePage, 0, szSrcStr, -1, pszDestStr, 0);
		nSize = nLength * sizeof(WCHAR);

		if (nLength)
		{
			pszDestStr = new WCHAR[nSize];
		}
		else
		{
			pszDestStr = NULL;
		}
		if (NULL != pszDestStr)
		{
			if (0 == ::MultiByteToWideChar(uCodePage, 0, szSrcStr, -1, pszDestStr, nLength))
			{
				pszDestStr = NULL;
			}
			else
			{
				// do nothing
			}
		}
		else
		{
			// do nothing
		}
		return pszDestStr;
	}
	static	void	FreeString(LPVOID szStrBuf)
	{
		if (szStrBuf)	delete szStrBuf;
	}

private:
	const	WCHAR* m_pwStr;
	DWORD			m_dwLen;
};
class CUTF16
{
public:
	CUTF16(IN std::string str)
		: CUTF16(str.c_str())
	{

	}
	CUTF16(IN LPCSTR pStr)
	{
		m_pwStr = NULL;
		m_szNull[0] = NULL;

		if (pStr || *pStr)
		{
			int		nReqSize = ::MultiByteToWideChar(CP_UTF8, 0, pStr, -1, m_pwStr, NULL);
			if (0 == nReqSize)
				return;

			m_pwStr = (LPWSTR) new WCHAR[nReqSize];
			if (m_pwStr)
			{
				::MultiByteToWideChar(CP_UTF8, 0, pStr, -1, m_pwStr, nReqSize);
			}
		}
	}
	~CUTF16()
	{
		if (m_pwStr)	delete m_pwStr;
	}

	LPCWSTR	Get()
	{
		return m_pwStr ? m_pwStr : m_szNull;
	}

private:
	LPWSTR		m_pwStr;
	TCHAR		m_szNull[1];
};
class CUTF8
{
public:
	CUTF8(IN std::wstring str)
		: CUTF8(str.c_str())
	{

	}
	CUTF8(IN LPCWSTR pStr)
	{
		m_pStr = NULL;
		m_szNull[0] = NULL;

		if (pStr && *pStr)
		{
			int		nReqSize = WideCharToMultiByte(CP_UTF8, 0, pStr, -1, m_pStr, 0, NULL, NULL);
			if (0 == nReqSize)
				return;

			m_pStr = (LPSTR) new CHAR[nReqSize];
			if (m_pStr)
			{
				::WideCharToMultiByte(CP_UTF8, 0, pStr, -1, m_pStr, nReqSize, NULL, NULL);
			}
		}
	}
	CUTF8(IN LPCSTR pStr)
		: CUTF8(__wide(pStr))
	{

	}
	~CUTF8()
	{
		if (m_pStr)	delete m_pStr;
	}

	LPCSTR	Get()
	{
		return m_pStr ? m_pStr : m_szNull;
	}

private:
	LPSTR		m_pStr;
	CHAR		m_szNull[1];
};
class CErrorMessage {
public:
	CErrorMessage(IN DWORD dwError = 0) {
		m_dwError	= dwError;
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwError, 0, m_szErrMsg, sizeof(m_szErrMsg), NULL);
	}
	operator	DWORD() {
		return m_dwError;
	}
	operator	PCWSTR() {
		return m_szErrMsg;
	}
	operator	PCSTR() {
		return __ansi(m_szErrMsg);
	}
private:
	DWORD	m_dwError;
	WCHAR	m_szErrMsg[1024];
};
class CStringBuffer
{
public:
	CStringBuffer(IN DWORD dwSize = STRING_BUFFER_SIZE)
		:
		m_pBuf(NULL),
		m_dwSize(dwSize)
	{
		static	WCHAR	szNull[]	= _T("");
		m_pBuf		=  (WCHAR *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize);
		m_dwSize	= dwSize;
	}
	~CStringBuffer()
	{
		if (m_pBuf)	HeapFree(GetProcessHeap(), 0, m_pBuf);
	}
	operator	LPTSTR() {
		return m_pBuf;
	}
	operator	LPCTSTR() {
		return m_pBuf;
	}
	operator	DWORD() {
		return m_dwSize;
	}
	operator size_t() {
		return m_dwSize;
	}
	void* Buffer()
	{
		return  m_pBuf;
	}
	DWORD		Size()
	{
		return m_dwSize;
	}
private:
	TCHAR		*m_pBuf;
	DWORD		m_dwSize;
};
