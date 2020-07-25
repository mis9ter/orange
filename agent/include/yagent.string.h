#pragma once

#include <Windows.h>
#include <strsafe.h>
#include <tchar.h>
#include <string>

#define	STRING_BUFFER_SIZE	4096
#define	__ansi(str)			CANSI(str).Get()
#define __wide(str)			CWIDE(str).Get()
#define __utf8(str)			CUTF8(str).Get()
#define __utf16(str)		CUTF16(str).Get()
#define	__mstricmp(a,b)		_mbsicmp((const unsigned char*)(a? a:""), (const unsigned char*)(b? b:""))
#define	__mstrstr(a,b)		_mbsstr((const unsigned char*)(a? a:""), (const unsigned char*)(b? b:""))
#define _t(p)				_T(p)

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
class CStringBuffer
{
public:
	CStringBuffer(IN DWORD dwSize = STRING_BUFFER_SIZE)
		:
		m_pBuf(NULL),
		m_dwSize(dwSize)
	{
		static	TCHAR	szNull[]	= _T("");
		try
		{
			m_pBuf		= new TCHAR[dwSize];
			m_dwSize	= dwSize;
		}
		catch (std::exception& e)
		{
			m_pBuf		= szNull;
			puts(e.what());
		}
	}
	~CStringBuffer()
	{
		if (m_pBuf)	delete m_pBuf;
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