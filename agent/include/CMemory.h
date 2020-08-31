#pragma once
/*
	2019/10/11	TigerJ
	메모리 관리하기 귀찮아서 첫 삽 한번 떠준다.
	난 이제 메모리 해제 따윈 안 할거다.
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

#define MEMORY_FLAG_FREE		0x00000001		//	소멸자에서 메모리를 해제한다.
#define MEMORY_FLAG_NOT_FREE	0x00000002		//	소멸자에서 메모리를 해제하지 않는다.
#define MEMORY_CAUSE_SIZE		32				//	메모리 할당 시 그 출처 기록용

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
			{	//	인자 전달이 잘못된 경우. 
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
			{	//	인자 전달이 잘못된 경우. 
				__leave;
			}
			//	메모리 할당에 실패한 경우 이전 값을 그대로 유지하도록 한다.
			LPVOID	p = CMemoryOperator::Allocate(dwSize + 2);
			if (p)
			{	//	성공적으로 할당 받았으므로 기존 메모리는 삭제해도 된다.
				Free();
				m_pData = p, m_dwSize = dwSize;
				if( pData )	::CopyMemory(m_pData, pData, m_dwSize);
				//	데이터 영역에 텍스트 문이 복사될 경우 편의를 위해서 제일 끝에 UNICODE만큼 NULL을 넣어준다.
				*((char *)m_pData + m_dwSize) = NULL;
				*((char *)m_pData + m_dwSize + 1) = NULL;
			}
			else
			{	//	실패. 기존 메모리 계속 유지.
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
			그런데 과연 Free 시킬 필요가 있을까요?
			그냥 어딘가 ptr이 소속된 곳에서 없애주기만 한다면 자동으로 소멸될텐데.
		*/
		ptr->Free();
	}
private:
	CMemoryFactory()
	{

	}
};
