#include <stdint.h>
#pragma once

#define	UNIX_TIME_START			0x019DB1DED53E8000	//January 1, 1970 (start of Unix epoch) in "ticks"
#define	TICKS_PER_SECOND		10000000			//a tick is 100ns
#define TICKS_PER_MILISECOND	10000

class CTime
{
public:
	static	PCSTR	LaregInteger2LocalTimeString(LARGE_INTEGER* pSrc, PSTR pStr, IN DWORD dwSize) {
		SYSTEMTIME	st;
		LargeInteger2SystemTime(pSrc, &st, true);
		SystemTimeToString(&st, pStr, dwSize);
		return pStr;
	}
	static	void	LargeInteger2FileTime(LARGE_INTEGER* pSrc, FILETIME* pDest) {
		pDest->dwHighDateTime = pSrc->HighPart;
		pDest->dwLowDateTime = pSrc->LowPart;
	}
	static	int64_t	LargeInteger2UnixTimestamp(LARGE_INTEGER* pSrc) {
		return (pSrc->QuadPart - UNIX_TIME_START) / TICKS_PER_MILISECOND;
	}
	static	void	LargeInteger2SystemTime(LARGE_INTEGER* pSrc, SYSTEMTIME* pDest, bool bLocal = true) {
		FILETIME	ftTime, ftLocalTime;

		ftTime.dwHighDateTime = pSrc->HighPart;
		ftTime.dwLowDateTime = pSrc->LowPart;
		if (bLocal) {
			FileTimeToLocalFileTime(&ftTime, &ftLocalTime);
			FileTimeToSystemTime(&ftLocalTime, pDest);
		}
		else
			FileTimeToSystemTime(&ftTime, pDest);
	}

	static	void	DateTimeStringToSystemTime(IN LPCTSTR pStr, OUT SYSTEMTIME* p)
	{
		::ZeroMemory(p, sizeof(SYSTEMTIME));
		//	01234567890123456789012
		//	2018-03-14 12:23:45.678
		p->wYear	= (WORD)_ttoi(pStr);
		p->wMonth	= (WORD)_ttoi(pStr + 5);
		p->wDay		= (WORD)_ttoi(pStr + 8);
		p->wHour	= (WORD)_ttoi(pStr + 11);
		p->wMinute	= (WORD)_ttoi(pStr + 14);
		p->wSecond	= (WORD)_ttoi(pStr + 17);

		if (lstrlen(pStr) >= 20)
			p->wMilliseconds = (WORD)_ttoi(pStr + 20);
	}
	static	LPCWSTR	SystemTimeToString(IN SYSTEMTIME* p, OUT LPWSTR pStr, IN DWORD dwSize, IN bool bTimer = false)
	{
		if( bTimer )
			::StringCbPrintf(pStr, dwSize, _T("%02d:%02d:%02d.%03d"),
				p->wHour, p->wMinute, p->wSecond, p->wMilliseconds);
		else
			::StringCbPrintf(pStr, dwSize, _T("%04d-%02d-%02d %02d:%02d:%02d.%03d"),
				p->wYear, p->wMonth, p->wDay, p->wHour, p->wMinute, p->wSecond, p->wMilliseconds);
		return pStr;
	}
	static	LPCSTR	SystemTimeToString(IN SYSTEMTIME* p, OUT LPSTR pStr, IN DWORD dwSize)
	{
		::StringCbPrintfA(pStr, dwSize, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
			p->wYear, p->wMonth, p->wDay, p->wHour, p->wMinute, p->wSecond, p->wMilliseconds);
		return pStr;
	}
	static	LPCSTR	FileTimeToLocalTimeString(IN FILETIME* p, OUT LPSTR pStr, IN DWORD dwSize)
	{
		SYSTEMTIME	st;
		FILETIME	ft;
		::FileTimeToLocalFileTime(p, &ft);
		::FileTimeToSystemTime(&ft, &st);
		return SystemTimeToString(&st, pStr, dwSize);
	}
	static	LPCWSTR	FileTimeToLocalTimeString(IN FILETIME* p, OUT LPWSTR pStr, IN DWORD dwSize)
	{
		SYSTEMTIME	st;
		FILETIME	ft;
		::FileTimeToLocalFileTime(p, &ft);
		::FileTimeToSystemTime(&ft, &st);
		return SystemTimeToString(&st, pStr, dwSize);
	}
	static	LPCWSTR	FileTimeToSystemTimeString(IN FILETIME* p, OUT LPWSTR pStr, IN DWORD dwSize, IN bool bTimer)
	{
		SYSTEMTIME	st;
		::FileTimeToSystemTime(p, &st);
		return SystemTimeToString(&st, pStr, dwSize, bTimer);
	}
	static	int64_t	SystemTimeToUnixTimestamp(IN SYSTEMTIME* p)
	{
		FILETIME	ft;

		::SystemTimeToFileTime(p, &ft);
		LARGE_INTEGER li;
		li.LowPart = ft.dwLowDateTime;
		li.HighPart = ft.dwHighDateTime;

		//Convert ticks since 1/1/1970 into seconds
		return (li.QuadPart - UNIX_TIME_START) / TICKS_PER_MILISECOND;
	}
	static	void	UnixTimeStampToSystemTime(IN int64_t dwTimestamp, OUT SYSTEMTIME* p)
	{
		LARGE_INTEGER	li;
		FILETIME		ft;

		if (dwTimestamp > 9999999999)
			li.QuadPart = (dwTimestamp * TICKS_PER_MILISECOND) + UNIX_TIME_START;
		else
			li.QuadPart = (dwTimestamp * TICKS_PER_SECOND) + UNIX_TIME_START;
		ft.dwHighDateTime = li.HighPart;
		ft.dwLowDateTime = li.LowPart;

		::FileTimeToSystemTime(&ft, p);
	}
	static	void	UnixTimeStampToLocalTime(IN int64_t dwTimestamp, OUT SYSTEMTIME* p)
	{
		LARGE_INTEGER	li;
		FILETIME		ft[2];

		if (dwTimestamp > 9999999999)
			li.QuadPart = (dwTimestamp * TICKS_PER_MILISECOND) + UNIX_TIME_START;
		else
			li.QuadPart = (dwTimestamp * TICKS_PER_SECOND) + UNIX_TIME_START;

		ft[0].dwHighDateTime = li.HighPart;
		ft[0].dwLowDateTime = li.LowPart;

		::FileTimeToLocalFileTime(&ft[0], &ft[1]);
		::FileTimeToSystemTime(&ft[1], p);
	}
	static	int64_t	GetUnixTimestamp()
	{
		SYSTEMTIME	st;

		::GetSystemTime(&st);

		return SystemTimeToUnixTimestamp(&st);
	}
	static	int64_t	DateTimeStringToUnixTimestamp(IN LPCTSTR pStr)
	{
		SYSTEMTIME	st;

		DateTimeStringToSystemTime(pStr, &st);
		return SystemTimeToUnixTimestamp(&st);
	}
	static	int64_t	LocalTimeToUnixTimestamp(IN SYSTEMTIME* p)
	{
		FILETIME	st, ft;

		::SystemTimeToFileTime(p, &st);
		::LocalFileTimeToFileTime(&st, &ft);

		LARGE_INTEGER li;
		li.LowPart = ft.dwLowDateTime;
		li.HighPart = ft.dwHighDateTime;

		//Convert ticks since 1/1/1970 into seconds
		return (li.QuadPart - UNIX_TIME_START) / TICKS_PER_MILISECOND;
	}
	//	이 함수는 msec 단위로 리턴
	static	int64_t	LocalDateTimeStringToUnixTimeStamp(IN LPCTSTR pStr)
	{
		if (NULL == pStr || NULL == *pStr || lstrlen(pStr) < 19)
			return 0;

		SYSTEMTIME	st;

		DateTimeStringToSystemTime(pStr, &st);
		return LocalTimeToUnixTimestamp(&st);
	}
	static	LPCTSTR	GetSystemTimeString(OUT LPTSTR pStr, IN DWORD dwSize)
	{
		SYSTEMTIME	st;
		::GetSystemTime(&st);
		return SystemTimeToString(&st, pStr, dwSize);
	}
	static	LPCTSTR	GetLocalTimeString(OUT LPTSTR pStr, IN DWORD dwSize)
	{
		SYSTEMTIME	st;
		::GetLocalTime(&st);
		return SystemTimeToString(&st, pStr, dwSize);
	}
	static	LPCSTR	GetLocalTimeString(OUT LPSTR pStr, IN DWORD dwSize)
	{
		SYSTEMTIME	st;
		::GetLocalTime(&st);
		return SystemTimeToString(&st, pStr, dwSize);
	}
	static	int64_t GetSystemTimeAsUnixTimestamp()
	{
		//Get the number of seconds since January 1, 1970 12:00am UTC
		//Code released into public domain; no attribution required.

		FILETIME ft;
		GetSystemTimeAsFileTime(&ft); //returns ticks in UTC

									  //Copy the low and high parts of FILETIME into a LARGE_INTEGER
									  //This is so we can access the full 64-bits as an Int64 without causing an alignment fault
		LARGE_INTEGER li;
		li.LowPart = ft.dwLowDateTime;
		li.HighPart = ft.dwHighDateTime;

		//Convert ticks since 1/1/1970 into seconds
		return (li.QuadPart - UNIX_TIME_START) / TICKS_PER_SECOND;
	}
	static	INT64	GetBootTimestamp()
	{
		return (GetUnixTimestamp() - GetTickCount64());
	}
	static	void	GetBootTime(OUT SYSTEMTIME * p)
	{
		GetBootLocalTime(p);
	}
	static	void	GetBootSystemTime(OUT SYSTEMTIME * p)
	{
		DWORD64		dwTimestamp = GetUnixTimestamp() - ::GetTickCount64();
		UnixTimeStampToSystemTime(dwTimestamp, p);
	}
	static	void	GetBootLocalTime(OUT SYSTEMTIME * p)
	{
		DWORD64		dwTimestamp = GetUnixTimestamp() - ::GetTickCount64();
		UnixTimeStampToLocalTime(dwTimestamp, p);
	}
private:

};
