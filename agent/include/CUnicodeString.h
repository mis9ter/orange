#pragma once

#define CUNICODE_MAX_STRING_SIZE	1024


class CWSTRBuffer
{
	const	ULONG		memory_tag = 'CWSB';

public:
	CWSTRBuffer(IN size_t cbSize = CUNICODE_MAX_STRING_SIZE)
		:
		m_pStr(CMemory::AllocateString(NonPagedPoolNx, cbSize,memory_tag)),
		m_cbSize(cbSize)
	{
		
		if (m_pStr)	RtlZeroMemory(m_pStr, m_cbSize);
		else			m_cbSize = 0;
	}
	~CWSTRBuffer()
	{
		Clear();
	}
	void	Clear()
	{
		if( m_pStr )	CMemory::FreeString(m_pStr);
		m_pStr = NULL, 	m_cbSize = 0;
	}
	operator PWSTR()
	{
		return	m_pStr;
	}
	operator PCWSTR()
	{
		return	m_pStr;
	}
	size_t	CbSize()
	{
		return m_cbSize;
	}
	size_t	CchSize()
	{
		return (m_cbSize / sizeof(WCHAR));
	}
private:
	LPWSTR			m_pStr;
	size_t			m_cbSize;
};
class CWSTR
{
public:
	CWSTR(IN PUNICODE_STRING p, IN POOL_TYPE type = NonPagedPoolNx)
		:
		CWSTR()
	{
		//__log("%-20s %wZ", __FUNCTION__, p);
		if( p )
		{
			m_cbSize	= p->Length + sizeof(WCHAR);
			m_pStr		= CMemory::AllocateString(type, m_cbSize, 'cucs');
			if (m_pStr)
			{
				if (NT_SUCCESS(RtlStringCbCopyUnicodeString(m_pStr, m_cbSize, p)))
				{

				}
				else
				{
					__log("%s RtlStringCbCopyUnicodeString() failed.", __FUNCTION__);
					Clear();
				}
			}
			else
			{
				__log("%s allocation(1) failed.", __FUNCTION__);
				Clear();
			}
		}
		else
		{
			__log("%s PUNICODE_STRING is null.", __FUNCTION__);
		}
	}
	CWSTR(IN PCWSTR p, IN POOL_TYPE type = NonPagedPoolNx)
		:
		CWSTR()
	{
		//__log("%-20s %ws", __FUNCTION__, p);
		if (p)
		{
			size_t	nSize	= 0;
			
			if (NT_SUCCESS(RtlStringCbLengthW(p, CUNICODE_MAX_STRING_SIZE, &nSize)))
			{
				m_cbSize = nSize + sizeof(WCHAR);
				m_pStr = CMemory::AllocateString(type, m_cbSize, 'cucs');
				if (m_pStr)
				{
					if (NT_SUCCESS(RtlStringCbCopyW(m_pStr, m_cbSize, p)))
					{

					}
					else
					{
						__log("%s RtlStringCbCopyW() failed.", __FUNCTION__);
						Clear();
					}
				}
				else
				{
					__log("%s allocation(2) failed.", __FUNCTION__);
					Clear();
				}
			}
			else
			{
				__log("%s RtlCbLengthW() failed.", __FUNCTION__);
			}
		}
		else
		{
			__log("%s PCWSTR is null.", __FUNCTION__);
		}
	}
	~CWSTR()
	{
		if( NULL == m_pStr )	__log("%-20s %ws", __FUNCTION__, m_pStr);
		Clear();
	}
	void		Clear()
	{
		if (m_pStr)	CMemory::FreeString(m_pStr), m_pStr = NULL;
		m_cbSize = 0;
	}
	operator LPCWSTR()
	{
		return Get();
	}
	PCWSTR		Get()
	{
		return (m_pStr && m_cbSize)? m_pStr : m_szNull;
	}
	size_t		CbSize()
	{
		return m_cbSize;
	}

private:
	CWSTR()
		: m_pStr(NULL), m_cbSize(0)
	{
		m_szNull[0] = NULL;
	}

	PWSTR		m_pStr;
	WCHAR		m_szNull[1];
	size_t		m_cbSize;
};
#define	__cwstr(p)	CWSTR(p).Get()

class CUnicodeString
{
public:
	CUnicodeString(IN PCUNICODE_STRING p, IN POOL_TYPE type)
		:	CUnicodeString()
	{
		SetString(p, NULL, type);
	}
	CUnicodeString(IN LPCWSTR p, IN POOL_TYPE type, IN size_t cbMaxSize = CUNICODE_MAX_STRING_SIZE)
		: CUnicodeString()
	{
		UNREFERENCED_PARAMETER(cbMaxSize);
		SetString(p, type);
	}
	CUnicodeString()
		:	m_pString(NULL), m_cbSize(0), m_szNull(L"")
	{
		UnsetString();
	}
	~CUnicodeString()
	{	
		UnsetString();
	}
	UNICODE_STRING &	operator = (const UNICODE_STRING & rhs)
	{
		UNREFERENCED_PARAMETER(rhs);
		return m_unicodestring;
	}
	CUnicodeString &	operator = (LPWSTR rhs)
	{
		SetString(rhs, PagedPool);
		return *this;
	}
	operator PUNICODE_STRING()
	{
		return &m_unicodestring;
	}
	operator PCWSTR()
	{
		return m_pString? m_pString : m_szNull;
	}
	void				UnsetString()
	{
		if (m_pString)
		{
			CMemory::FreeString(m_pString);
		}
		m_pString = NULL, m_cbSize = 0;
		RtlInitUnicodeString(&m_unicodestring, NullString());
	}
	bool				SetString(IN PCUNICODE_STRING p, IN PCWSTR p2, IN POOL_TYPE type)
	{
		//	p2	이 값이 존재할 경우 p + p2의 형태로 문자열을 만든다. 
		NTSTATUS	status	= STATUS_UNSUCCESSFUL;

		UnsetString();
		if( NULL == p || 0 == p->Length || 0 == p->MaximumLength )	return false;
		size_t	cbSize2	= p2? wcslen(p2) : 0;
		size_t	cbSize	= p->Length + sizeof(wchar_t) + cbSize2;
		LPWSTR	pString	= NULL;
		__try
		{
			pString = CMemory::AllocateString(type, cbSize, 'cucs');
			if( NULL == pString )	__leave;
			if (NT_FAILED(status = RtlStringCbCopyUnicodeString(pString, cbSize, p))) {
				__log("%s cbSize=%d, cbSize2=%d", __FUNCTION__, (int)cbSize, (int)cbSize2);
				__log("  RtlStringCbCopyUnicodeString() failed.");
				__log("  p=%wZ (%d)", p, (int)p->Length);

				__leave;			
			}
			if (p2 && NT_FAILED(status = RtlStringCbCatW(pString, cbSize, p2)) ) {
				__log("%s RtlStringCbCatW() failed.", __FUNCTION__); 
				__leave;
			}
			RtlInitUnicodeString(&m_unicodestring, pString);
			m_pString = pString, m_cbSize = cbSize;
		}
		__finally
		{
			if (NT_FAILED(status))
			{
				if( pString )	CMemory::FreeString(pString);
				UnsetString();
			}
		}
		return NT_SUCCESS(status);
	}
	bool				SetString(IN LPCWSTR p, IN POOL_TYPE type)
	{
		UnsetString();
		if( NULL == p )	return false;
		size_t		cbSize	= 0;
		m_pString	= CMemory::AllocateString(type, p, &cbSize, 'SEST');
		if (m_pString)
		{
			m_cbSize	= cbSize;
			RtlInitUnicodeString(&m_unicodestring, m_pString);
			return true;
		}
		else
			UnsetString();
		return false;
	}
private:
	UNICODE_STRING		m_unicodestring;
	LPWSTR				m_pString;
	size_t				m_cbSize;
	wchar_t				m_szNull[1];

	LPCWSTR				NullString()
	{
		return m_szNull;
	}
};

