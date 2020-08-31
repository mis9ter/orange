#pragma once
/*
	2019/10/11	TigerJ
	�޸� �����ϱ� �����Ƽ� ù �� �ѹ� ���ش�.
	�� ���� �޸� ���� ���� �� �ҰŴ�.
*/
#define CONSOLE_LOG				0

class CMemoryOperator
{
public:
	static	LPVOID	Allocate(IN DWORD dwSize)
	{
		LPVOID	pData = ::HeapAlloc(::GetProcessHeap(), HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, dwSize);
#if defined(_CONSOLE) && defined(CONSOLE_LOG) && 1 == CONSOLE_LOG
		printf("%-40s %p %d\n", __FUNCTION__, pData, dwSize);
#endif
		return pData;
	}
	static	void	Free(IN LPVOID pData)
	{
		if (pData) {
			::HeapFree(::GetProcessHeap(), HEAP_NO_SERIALIZE, pData);
#if defined(_CONSOLE) && defined(CONSOLE_LOG) && 1 == CONSOLE_LOG
			printf("%-40s %p\n", __FUNCTION__, pData);
#endif
		}
	}
private:

};

#define MEMORY_FLAG_FREE		0x00000001		//	�Ҹ��ڿ��� �޸𸮸� �����Ѵ�.
#define MEMORY_FLAG_NOT_FREE	0x00000002		//	�Ҹ��ڿ��� �޸𸮸� �������� �ʴ´�.
#define MEMORY_CAUSE_SIZE		32				//	�޸� �Ҵ� �� �� ��ó ��Ͽ�

class CMemory
{
public:
	CMemory(IN LPCSTR pCause, IN DWORD dwFlag = MEMORY_FLAG_NOT_FREE, IN LPVOID pData = NULL, IN DWORD dwSize = 0)
		:
		m_pData(NULL),
		m_dwSize(0),
		m_dwFlag(MEMORY_FLAG_NOT_FREE),
		m_szCause("")
	{
		UNREFERENCED_PARAMETER(dwFlag);
		::StringCbCopyA(m_szCause, sizeof(m_szCause), pCause ? pCause : "");
		if (dwSize)		Allocate(pData, dwSize);
	}
	~CMemory()
	{
		char	szCause[MEMORY_CAUSE_SIZE + 1];
		::StringCbCopyA(szCause, sizeof(szCause), m_szCause);
		Free();
	}
	bool			Transfer(IN DWORD dwFlag, IN LPCVOID pData, IN DWORD dwSize)
	{
		bool	bRet = false;
		__try
		{
			if (0 == dwSize)
			{	//	���� ������ �߸��� ���. 
				__leave;
			}
			Free();
			m_pData = (LPVOID)pData, m_dwSize = dwSize;
			m_dwFlag = dwFlag, bRet = true;
		}
		__finally
		{
		}
		return bRet;
	}
	bool			Allocate(IN LPCVOID pData, IN DWORD dwSize)
	{
#if defined(_CONSOLE) && defined(CONSOLE_LOG) && 1 == CONSOLE_LOG
		printf("%-40s %p %d\n", __FUNCTION__, pData, dwSize);
#endif
		bool	bRet = false;
		__try
		{
			if (0 == dwSize)
			{	//	���� ������ �߸��� ���. 
				__leave;
			}
			//	�޸� �Ҵ翡 ������ ��� ���� ���� �״�� �����ϵ��� �Ѵ�.
			LPVOID	p = CMemoryOperator::Allocate(dwSize + 2);
			if (p)
			{	//	���������� �Ҵ� �޾����Ƿ� ���� �޸𸮴� �����ص� �ȴ�.
				Free();
				m_pData = p, m_dwSize = dwSize;
				if( pData )	::CopyMemory(m_pData, pData, m_dwSize);
				//	������ ������ �ؽ�Ʈ ���� ����� ��� ���Ǹ� ���ؼ� ���� ���� UNICODE��ŭ NULL�� �־��ش�.
				*((char *)m_pData + m_dwSize) = NULL;
				*((char *)m_pData + m_dwSize + 1) = NULL;
			}
			else
			{	//	����. ���� �޸� ��� ����.
				__leave;
			}
			m_dwFlag = MEMORY_FLAG_FREE, bRet = true;
		}
		__finally
		{	}
		return bRet;
	}
	void			Free()
	{
		if (m_pData && MEMORY_FLAG_FREE == m_dwFlag)
		{
			CMemoryOperator::Free(m_pData);
#if defined(_CONSOLE) && defined(CONSOLE_LOG) && 1 == CONSOLE_LOG
			printf("%-40s %p %d\n", __FUNCTION__, m_pData, m_dwSize);
#endif
		}
		m_pData = NULL, m_dwSize = 0, m_dwFlag = MEMORY_FLAG_NOT_FREE;
	}
	LPVOID			Data()
	{
		return m_pData;
	}
	DWORD			Size()
	{
		return m_dwSize;
	}
private:
	LPVOID			m_pData;
	DWORD			m_dwSize;
	DWORD			m_dwFlag;
	char			m_szCause[MEMORY_CAUSE_SIZE+1];
};

typedef std::shared_ptr<CMemory>	CMemoryPtr;
class CMemoryFactory
{
public:
	~CMemoryFactory()
	{

	}
	static	CMemoryFactory *	Instance()
	{
		static	CMemoryFactory	p;
		return &p;
	}
	CMemoryPtr					AllocatePtr(IN LPCSTR pCause, IN LPVOID pData, IN DWORD dwSize)
	{
		CMemoryPtr ptr = std::make_shared<CMemory>(pCause, MEMORY_FLAG_FREE, pData, dwSize);
		if (NULL == ptr->Data())	return nullptr;
		return ptr;
	}
	void						FreePtr(IN CMemoryPtr ptr)
	{
		/*
			�׷��� ���� Free ��ų �ʿ䰡 �������?
			�׳� ��� ptr�� �Ҽӵ� ������ �����ֱ⸸ �Ѵٸ� �ڵ����� �Ҹ���ٵ�.
		*/
		ptr->Free();
	}
private:
	CMemoryFactory()
	{

	}
};
