#pragma once
#define CMEMORY_MAX_STRING_SIZE	1024

#define __case_copynameA(name,buf,size)	case name: RtlStringCbCopyA(buf,size,#name)

class CMemory
{
public:
/*
	static	void *	__cdecl	operator	new(size_t n, POOL_TYPE pool, ULONG tag)
	{
		if (0 == tag)
		{
			__log("%s untagged memory allocation: size=%d", __FUNCTION__, (int)n);
			return NULL;
		}
		PVOID	p = ExAllocatePoolWithTag(pool, n, tag);
		return p;
	}
	static	void	__cdecl	operator	delete(void * p, ULONG tag)
	{
		if (p)
		{
			if (tag)	ExFreePoolWithTag(p, tag);
			else
				ExFreePool(p);
		}
	}
	static	void *	__cdecl	operator	new[](size_t n, POOL_TYPE pool, ULONG tag)
	{
		if (0 == tag)
		{
			__log("%s untagged memory allocation: size=%d", __FUNCTION__, (int)n);
			return NULL;
		}
		PVOID	p = ExAllocatePoolWithTag(pool, n, tag);
		return p;
	}
		static	void	__cdecl	operator	delete[](void * p, ULONG tag)
	{
		if (p)
		{
			if (tag)	ExFreePoolWithTag(p, tag);
			else
				ExFreePool(p);
		}
	}
*/
 	static	void *	Allocate(POOL_TYPE pool, size_t n, ULONG tag)
	{
		if (0 == tag)
		{
			__log("%s untagged memory allocation: size=%d", __FUNCTION__, (int)n);
			return NULL;
		}
		PVOID	p = ExAllocatePoolWithTag(pool, n, tag);
		return p;
	}
	static	void	Free(void * p, ULONG tag = 0)
	{
		if (p)
		{
			if (tag)	ExFreePoolWithTag(p, tag);
			else
				ExFreePool(p);
		}
	}
	static	bool	AllocateUnicodeString(
		IN POOL_TYPE type, 
		OUT PUNICODE_STRING pDest, 
		IN PCUNICODE_STRING pSrc,
		IN ULONG			nTag = 'AUS1'
	)
	{
		bool	bRet	= false;
		if (NULL == pSrc || NULL == pDest)	return bRet;
		__try
		{
			pDest->Buffer = (PWSTR)ExAllocatePoolWithTag(type, pSrc->MaximumLength, nTag);
			if (NULL == pDest->Buffer)	{
				__log("%-20s memoty(%d) allocation failed. size=%d", 
					__FUNCTION__, type, pSrc->MaximumLength);
				__leave;
			}
			pDest->MaximumLength	= pSrc->MaximumLength;
			if( NT_SUCCESS(RtlUnicodeStringCopy(pDest, pSrc)))
			{
				bRet	= true;
			}
			else
			{
				__log("%s RtlUnicodeStringCopy() failed.", __FUNCTION__);
				FreeUnicodeString(pDest);
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			
		}
		return bRet;
	}
	static	bool	AllocateUnicodeString(IN POOL_TYPE type, 
		OUT PUNICODE_STRING pDest, IN PCWSTR pSrc)
	{
		bool	bRet	= false;
		if( NULL == pSrc || NULL == pDest)	return NULL;
		__try 
		{
			size_t	cbSize	= 0;
			
			if( !NT_SUCCESS(RtlStringCbLengthW(pSrc, CMEMORY_MAX_STRING_SIZE, &cbSize)))
			{
				__log("RtlStringCbLengthW() failed.");
				__leave;
			}
			pDest->Buffer	= (PWSTR)ExAllocatePoolWithTag(type, cbSize + sizeof(WCHAR), 'AUS2');		
			if( NULL == pDest->Buffer )	{
				__log("%-20s memory(%d) allocation failed. size=%d", 
					__FUNCTION__, type, (int)(cbSize + sizeof(WCHAR)));
				__leave;
			}
			pDest->MaximumLength	= (USHORT)(cbSize + sizeof(WCHAR));
			if( !NT_SUCCESS(RtlUnicodeStringCopyString(pDest, pSrc)))
			{
				__log("RtlUnicodeStringCopyString() failed.");
				FreeUnicodeString(pDest);
				__leave;
			}
			bRet	= true;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			
		}
		return bRet;
	}
	static	void	FreeUnicodeString(PUNICODE_STRING p, bool bLog = false)
	{
		if( NULL == p )	return;
		__try 
		{
			if( bLog )	__dlog("%s p->Buffser=%p, p->Length=%d", 
							__FUNCTION__, p->Buffer, p->Length);
			if (p->MaximumLength > 0 && p->Buffer)
				ExFreePool(p->Buffer);
			p->Buffer			= NULL;
			p->Length			= 0;
			p->MaximumLength	= 0;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {

		}
	}
	static	LPWSTR	AllocateString(IN POOL_TYPE type, IN size_t cbSize, IN ULONG nTag)
	{
		WCHAR	*p = NULL;

		if (0 == nTag)
		{
			__log("%s tag is 0.", __FUNCTION__);
			return NULL;
		}
		__try
		{
			p = (WCHAR *)ExAllocatePoolWithTag(type, cbSize + sizeof(WCHAR), nTag);
			if (NULL == p) {
				__log("%-20s memory(%d) is not allocated. size=%d", 
					__FUNCTION__, type, (int)(cbSize+sizeof(WCHAR)));
				__leave;
			}
			p[cbSize / sizeof(WCHAR)] = NULL;
		}
		__finally
		{
		
		}
		return p;
	}
	static	LPWSTR	AllocateString(IN POOL_TYPE type, IN LPCWSTR pSrcStr, OUT size_t * pStrSize, IN ULONG nTag)
	{
		WCHAR		*p = NULL;
		size_t		cbMaxSize = 0, cbSize = 0;
		NTSTATUS	status;

		if (0 == nTag)
		{
			__log("%s tag is 0.", __FUNCTION__);
			return NULL;
		}
		__try
		{
			if (pSrcStr)
			{
				if (!NT_SUCCESS(status = RtlStringCbLengthW(pSrcStr, 1024, &cbSize)))
				{
					__log("RtlStringCbLengthW() failed. status=%08x, pSrcStr=%ws", status, pSrcStr);
					__leave;
				}
				cbMaxSize = cbSize + sizeof(WCHAR);
			}
			else
			{
				cbMaxSize = 0;
			}
			if (pStrSize)	*pStrSize = cbSize;
			p = cbMaxSize ? (WCHAR *)ExAllocatePoolWithTag(type, cbMaxSize, nTag) : NULL;
			if (NULL == p) {
				__log("%-20s memory(%d) is not allocated. size=%d", 
					__FUNCTION__, type, (int)(cbMaxSize));
				__leave;
			}
			RtlCopyMemory(p, pSrcStr, cbSize);
			p[cbSize / sizeof(WCHAR)] = NULL;
		}
		__finally {}
		return p;
	}
	static	void	FreeString(IN LPWSTR p)
	{
		if (p)
		{
			ExFreePool(p);
		}
	}

private:

};

