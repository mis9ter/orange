#pragma once

#include <WinInet.h>

#pragma comment(lib, "wininet.lib")

#define INTERNET_INVALID_PORT_NUMBER    0           // use the protocol-specific default

#define INTERNET_DEFAULT_FTP_PORT       21          // default for FTP servers
#define INTERNET_DEFAULT_GOPHER_PORT    70          //    "     "  gopher "
#define INTERNET_DEFAULT_HTTP_PORT      80          //    "     "  HTTP   "
#define INTERNET_DEFAULT_HTTPS_PORT     443         //    "     "  HTTPS  "
#define INTERNET_DEFAULT_SOCKS_PORT     1080        // default for SOCKS firewall servers.

typedef bool	(__stdcall * CompressCallback)(IN LPVOID param, IN DWORD dwTotalSize, IN DWORD dwCurrentSize);
typedef bool	(__stdcall * RequestFileCallback)(LPVOID ptr, LPCTSTR lpPath, IN DWORD dwTotalBytes, IN DWORD dwCurrentBytes);

class CHttp
{
public:
	CHttp() {
	}
	~CHttp() {
	
	}

	static	bool	RequestFile
	(
		IN	LPCTSTR				pUrl,		//
		OUT	LPCTSTR				lpPath,		// 응답은 파일로 만들어진다.
		IN	DWORD				*pdwSize,	// IN lpPath의 크기, OUT 응답 파일의 크기 
		IN	RequestFileCallback pCallback,	
		IN	LPVOID				ptr
	)
	{
		bool		bRet		= false;
		HINTERNET	hInternet	= NULL,
			hConnect	= NULL,
			hRequest	= NULL;
		HANDLE		hFile		= INVALID_HANDLE_VALUE;
		TCHAR		szFileName[MAX_PATH]	= _T("");
		DWORD		dwFlags		= 0;
		DWORD		dwSize;

		TCHAR		szProtocol[10]	= _T(""),
			szUser[32]		= _T(""),
			szPasswd[32]	= _T(""),
			szAddress[256]	= _T(""),
			szUri[1024]		= _T("");
		DWORD		dwPort			= 0;

		__try
		{
			__w3curlparse(pUrl, szProtocol, sizeof(szProtocol), szUser, sizeof(szUser), szPasswd, sizeof(szPasswd),
				szAddress, sizeof(szAddress), &dwPort, szUri, sizeof(szUri));
			hInternet = ::InternetOpen
			(
				_T("YAGENT"),//AGENT_NAME, 
				INTERNET_OPEN_TYPE_PRECONFIG, // Use registry settings. 
				NULL, // Proxy name. NULL indicates use default.
				NULL, // List of local servers. NULL indicates default. 
				0
			) ;
			if( NULL == hInternet )
			{
				//_log(_T("error: InternetOpen() failed. url = [%s], code = %d"), pUrl, ::GetLastError());
				__leave;
			}

			hConnect	= ::InternetConnect(hInternet, szAddress, (WORD)dwPort, szUser, szPasswd, 
				_tcsicmp(szProtocol, _T("ftp"))? INTERNET_SERVICE_HTTP : INTERNET_SERVICE_FTP,
				dwFlags, (DWORD_PTR)ptr);
			if( NULL == hConnect )
			{
				__leave;
			}
			dwFlags	= INTERNET_FLAG_NO_CACHE_WRITE|INTERNET_FLAG_PRAGMA_NOCACHE|INTERNET_FLAG_RELOAD;
			if( !_tcsicmp(szProtocol, _T("https")) )
			{
				dwFlags	|= (INTERNET_FLAG_SECURE|INTERNET_FLAG_IGNORE_CERT_CN_INVALID|INTERNET_FLAG_IGNORE_CERT_DATE_INVALID);
			}
			::DeleteUrlCacheEntry(pUrl);
			hRequest	= ::HttpOpenRequest(hConnect, NULL, szUri, NULL, NULL, NULL, dwFlags, 0);
			if( NULL == hRequest )
			{
				__leave;
			}
			dwSize	= sizeof(dwFlags);
			if( ::InternetQueryOption(hRequest, INTERNET_OPTION_SECURITY_FLAGS, (LPVOID)&dwFlags, &dwSize) )
			{
				dwFlags	|= (
					SECURITY_FLAG_IGNORE_UNKNOWN_CA|
					SECURITY_FLAG_IGNORE_REVOCATION|
					SECURITY_FLAG_IGNORE_WRONG_USAGE|
					SECURITY_FLAG_IGNORE_REDIRECT_TO_HTTP|
					SECURITY_FLAG_IGNORE_REDIRECT_TO_HTTPS|
					SECURITY_FLAG_IGNORE_CERT_CN_INVALID|
					SECURITY_FLAG_IGNORE_CERT_DATE_INVALID
					);
				if( ::InternetSetOption(hRequest, INTERNET_OPTION_SECURITY_FLAGS, &dwFlags, sizeof(dwFlags)) )
				{
					//_log(_T("set option"));	
				}
				else
				{
					//_log(_T("error: InternetSetOption() failed. code = %d"), ::GetLastError());
				}
			}
			else
			{
				//_log(_T("error: InternetQueryOption() failed. code = %d"), ::GetLastError());
			}
			//LPCTSTR	pHeader	= _T("Content-Type: application/x-www-form-urlencoded");
			if( ::HttpSendRequest(hRequest, NULL, 0, NULL, 0) )
			{
				// OK!!
			}
			else
			{
				//DWORD	dwError	= ::GetLastError();
				//::SetLastError(dwError);
				//_log(_T("error: HttpSendRequestEx() failed. code = %d"), dwError);
			}
			TCHAR			szStr[256]	= {NULL,};
			unsigned long	nRead		= sizeof(szStr);
			if( ::HttpQueryInfo(hRequest, HTTP_QUERY_STATUS_CODE, szStr, &nRead, NULL) )
			{
				//_log(_T("HTTP STATUS CODE:%s"), szStr);

				// 2013/11/22	Kim beomjin
				// 기존은 HTTP_STATUS_NOT_FOUND인 경우에만 파일 생성 실패로 처리하도록 했다.
				// 그런데 HTTP_STATUS_SERVER_ERROR 와 같은 에러도 생기더라.
				// 하여 HTTP_STATUS_OK 의 값을 명시적으로 받은 경우에만 성공으로 처리하고
				// 나머지 코드들은 모두 실패로 간주하도록 수정한다.
				// 
				if( HTTP_STATUS_OK != _ttoi(szStr) )
					//if( HTTP_STATUS_NOT_FOUND == _ttoi(szStr) )
				{
					//_log(_T("error: url[%s] is not found."), pUrl);
					// FILE IS NOT FOUND
					__leave;
				}
			}
			else
			{
				//_log(_T("error: HttpQueryInfo() #1 failed. code = %d"), ::GetLastError());
				__leave;
			}

			DWORD	dwBytes		= QueryContentSizeInBytes(hRequest);
			DWORD	dwReadBytes	= 0,
				dwWrittenBytes	= 0;

			char	szBuf[4096]	= {0, };

			//_log(_T("BYTES:%d"), dwBytes);
			//_log(_T("creating file %s"), lpPath);
			//CreateFileDirectory(lpPath);

			hFile	= ::CreateFile(lpPath, GENERIC_READ|GENERIC_WRITE, 
				FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, 
				FILE_ATTRIBUTE_NORMAL, NULL);

			if( INVALID_HANDLE_VALUE == hFile )
			{
				//_log(_T("error: CreateFile(%s) failed. code = %d"), lpPath, ::GetLastError());
				__leave;
			}

			DWORD	dwResponseBytes	= 0;
			bool	bRead		= true;

			while( true )
			{
				bRead	= ::InternetReadFile(hRequest, szBuf, sizeof(szBuf), &dwReadBytes)? true : false;
				if( bRead && dwReadBytes )
				{

				}
				else
				{
					break;
				}

				if( ::WriteFile(hFile, szBuf, dwReadBytes, &dwWrittenBytes, NULL) && 
					dwReadBytes == dwWrittenBytes )
				{
					dwResponseBytes	+= dwWrittenBytes;
				}
				else
				{
					//_log(_T("error: WriteFile() failed. code = %d"), ::GetLastError());
					__leave;
				}

				if( pCallback )
				{
					if( false == pCallback(ptr, lpPath, dwBytes, dwResponseBytes) )
					{
						//_log(_T("error: pCallback returns false."));
						break;
					}
				}
			}

			::HttpEndRequest(hRequest, NULL, NULL, 0);

			if( pdwSize )
			{
				*pdwSize	= dwResponseBytes;
			}

			//_log(_T("download [%s] to [%s]"), pUrl, lpPath);

			bRet	= true;

		}
		__finally
		{
			if( INVALID_HANDLE_VALUE != hFile )
			{
				::CloseHandle(hFile);
			}

			if( hRequest )
			{
				::InternetCloseHandle(hRequest);
			}

			if( hConnect )
			{
				::InternetCloseHandle(hConnect);
			}

			if( hInternet )
			{
				::InternetCloseHandle(hInternet);
			}
		}
		return bRet;

	}

	static	void __w3curlparse
	(
		IN	LPCTSTR		pUrl,
		OUT	LPTSTR		pProtocol,
		IN	DWORD		dwProtocolSize,
		OUT	LPTSTR		pUser,
		IN	DWORD		dwUserSize,
		OUT	LPTSTR		pPasswd,
		IN	DWORD		dwPasswdSize,
		OUT	LPTSTR		pAddress,
		IN	DWORD		dwAddressSize,
		OUT	DWORD		*pPort,
		OUT	LPTSTR		pUri,
		IN	DWORD		dwUriSize
	)
	{
		long		nPos	= 0;
		long		nsp=0, usp=0;
		WCHAR		szProtocol[10]	= L"";

		while( wcslen(pUrl) > 0 && nPos < lstrlen(pUrl) && _T(':') != *(pUrl + nPos) )
		{
			++nPos;
		}

		if( _T('/') == *(pUrl + nPos + 1) )
		{
			// protocol
			::StringCchCopyN(szProtocol, sizeof(szProtocol)/sizeof(TCHAR), pUrl, nPos);

			if( pProtocol && dwProtocolSize )
			{
				::StringCbCopy(pProtocol, dwProtocolSize, szProtocol);
			}
			usp	=	nsp	=	nPos	+=	3;
		}
		else
		{
			// host
			::StringCbCopy(szProtocol, sizeof(szProtocol), _T("http"));
			if( pProtocol && dwProtocolSize )
			{
				::StringCbCopy(pProtocol, dwProtocolSize, szProtocol);
			}
			usp	= nsp	= nPos	= 0;
		}

		while( lstrlen(pUrl) > 0 && usp < lstrlen(pUrl) && _T('@') != *(pUrl + usp) )
		{
			++usp;
		}

		if( usp < lstrlen(pUrl) )
		{ 
			// find username and find password
			long ssp=nsp;

			while( *(pUrl + ssp) && lstrlen(pUrl) > 0 && nPos < lstrlen(pUrl) && _T(':') != *(pUrl+ssp) )
			{
				++ssp;
			}

			if( ssp < usp ) 
			{
				// find
				if( pUser && dwUserSize )
				{
					::StringCchCopyN(pUser, dwUserSize/sizeof(TCHAR), pUrl + nsp, ssp - nsp);
				}
				if( pPasswd && dwPasswdSize )
				{
					::StringCchCopyN(pPasswd, dwPasswdSize/sizeof(TCHAR), pUrl + ssp + 1, usp - ssp - 1);
				}
			}

			nsp	=	nPos	=	usp + 1;
		}

		while( lstrlen(pUrl) > 0 && nPos < lstrlen(pUrl) && _T('/') != *(pUrl + nPos) )
		{
			++nPos;
		}

		long	nf		= nsp;
		DWORD	dwPort	= 0;

		for( ; nf <= nPos ; nf++)
		{
			if( _T(':') == *(pUrl + nf) )
			{
				dwPort	= _ttoi(pUrl + nf + 1);
				break;
			}
		}

		if( dwPort )
		{
			if( pAddress && dwAddressSize )
			{
				::StringCchCopyN(pAddress, dwAddressSize/sizeof(TCHAR), pUrl + nsp, nf - nsp);
			}
		}
		else
		{
			if( !_tcsicmp(szProtocol, _T("https")) )
			{
				dwPort	= INTERNET_DEFAULT_HTTPS_PORT;
			}
			else if( !_tcsicmp(szProtocol, _T("ftp")) )
			{
				dwPort	= INTERNET_DEFAULT_FTP_PORT;
			}
			else
			{
				dwPort	= INTERNET_DEFAULT_HTTP_PORT;
			}
			if( pAddress && dwAddressSize )
			{
				::StringCchCopyN(pAddress, dwAddressSize/sizeof(TCHAR), pUrl + nsp, nPos - nsp);
			}
		}

		if( pPort )
		{
			*pPort	= dwPort;
		}

		if( nPos < lstrlen(pUrl) )
		{ 
			// find URI
			if( pUri && dwUriSize )
			{
				::StringCchCopyN(pUri, dwUriSize/sizeof(TCHAR), pUrl + nPos, lstrlen(pUrl) - nPos);
			}
		}
		else
		{
			if( pUri && dwUriSize )
			{
				::StringCbCopy(pUri, dwUriSize, _T("/"));
			}
		}
	}
	static int QueryContentSizeInBytes(HINTERNET hRequest)
	{
		if(NULL == hRequest) return -1;

		int urlContentSize = 0;
		DWORD size = sizeof(urlContentSize);
		BOOL re = FALSE;

		if( FALSE == ::HttpQueryInfo(
			hRequest,
			HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, //Retrieves the size of the resource, in bytes.
			&urlContentSize, 
			&size, 
			NULL) )
		{
			DWORD	dwError	= ::GetLastError();
			::SetLastError(dwError);
			if( ERROR_HTTP_HEADER_NOT_FOUND == dwError )
			{
				return 0;
			}
			//xcommon::_log(_T("HttpQueryInfo() #2 failed. code = %d"), dwError);
			return -2;
		}

		return urlContentSize;
	}
};

