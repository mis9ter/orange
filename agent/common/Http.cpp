#include "pch.h"
#include <wininet.h>

YAGENT_COMMON_BEGIN

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
	TCHAR		szProtocol[10]	= _T("");

	while( lstrlen(pUrl) > 0 && nPos < lstrlen(pUrl) && _T(':') != *(pUrl + nPos) )
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
static int	QueryContentSizeInBytes(HINTERNET hRequest)
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
bool		RequestFile
(
	IN	LPCTSTR		pUrl,		//
	OUT	LPCTSTR		lpPath,		// ������ ���Ϸ� ���������.
	IN	DWORD		*pdwSize,	// IN lpPath�� ũ��, OUT ���� ������ ũ�� 
	IN	LPVOID		pContext,
	IN	std::function<bool (LPVOID pContext, LPCWSTR lpPath, IN HANDLE hFile, IN DWORD dwTotalBytes, IN DWORD dwCurrentBytes)>	pCallback
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

	do
	{
		__w3curlparse(pUrl, szProtocol, sizeof(szProtocol), szUser, sizeof(szUser), szPasswd, sizeof(szPasswd),
			szAddress, sizeof(szAddress), &dwPort, szUri, sizeof(szUri));

		//_log(_T("PROTOCOL:%s"), szProtocol);
		//_log(_T("USER    :%s"), szUser);
		//_log(_T("PASSWD  :%s"), szPasswd);
		//_log(_T("ADDRESS :%s"), szAddress);
		//_log(_T("PORT    :%d"), dwPort);
		//_log(_T("URI     :%s"), szUri);

		hInternet = ::InternetOpen
		(
			_T("X/AGENT"),//AGENT_NAME, 
			INTERNET_OPEN_TYPE_PRECONFIG, // Use registry settings. 
			NULL, // Proxy name. NULL indicates use default.
			NULL, // List of local servers. NULL indicates default. 
			0
		) ;
		if( NULL == hInternet )
		{
			printf("error: InternetOpen() failed. url:[%ws], code:%d\n", pUrl, ::GetLastError());
			break;
		}

		hConnect	= ::InternetConnect(hInternet, szAddress, (WORD)dwPort, szUser, szPasswd, 
			_tcsicmp(szProtocol, _T("ftp"))? INTERNET_SERVICE_HTTP : INTERNET_SERVICE_FTP,
			dwFlags, (DWORD_PTR)pContext);
		if( NULL == hConnect )
		{
			CErrorMessage	err(GetLastError());
			printf("error: InternetConnect() failed.\n");
			printf("%ws\n", pUrl);
			printf("error:%d %s\n", (DWORD)err, (PCSTR)err);
			break;
		}

		/*
		2016/07/27	KimPro
		Ư��PC���� ������ ���� ������ �߻��ǰ� �ִ�.

		18:56 ������ error : HttpSendRequestEx() failed code = 12029
		18:56 ������ error:url[http://192.168.0.229/update/xupdate.ini] is not found
		18:56 ������ error: can not download profile [http://192.169.0.229/xupdate/xupdate.ini]
		18:57 ������ ��õ������ ��� uac�� ���� ���� ������Ʈ�� �Ǿ��ٰ� �մϴ�.
		19:00 ������ ERROR_INTERNET_CANNOT_CONNECT
		12029
		The attempt to connect to the server failed.

		������ ����?
		��� Ȯ���� �� �Ͽ����� �Ʒ��� ���� �ɼǿ� ������ �ξ���.


		dwFlags	= INTERNET_FLAG_NO_CACHE_WRITE|INTERNET_FLAG_PRAGMA_NOCACHE;
		*/
		dwFlags	= INTERNET_FLAG_NO_CACHE_WRITE|INTERNET_FLAG_PRAGMA_NOCACHE|INTERNET_FLAG_RELOAD;
		//dwFlags	= INTERNET_FLAG_NO_CACHE_WRITE|INTERNET_FLAG_PRAGMA_NOCACHE;

		if( !_tcsicmp(szProtocol, _T("https")) )
		{
			dwFlags	|= (INTERNET_FLAG_SECURE|INTERNET_FLAG_IGNORE_CERT_CN_INVALID|INTERNET_FLAG_IGNORE_CERT_DATE_INVALID);
		}

		::DeleteUrlCacheEntry(pUrl);

		hRequest	= ::HttpOpenRequest(hConnect, NULL, szUri, NULL, NULL, NULL, dwFlags, 0);
		if( NULL == hRequest )
		{
			CErrorMessage	err(GetLastError());
			printf("error: HttpOpenRequest() failed. pUrl:[%ws], flags:%08x, code:%d", 
				pUrl, dwFlags, (DWORD)err);
			break;
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
		}
		//LPCTSTR	pHeader	= _T("Content-Type: application/x-www-form-urlencoded");
		if( ::HttpSendRequest(hRequest, NULL, 0, NULL, 0) )
		{
			// OK!!
		}
		else
		{
			DWORD	dwError	= ::GetLastError();
			::SetLastError(dwError);
		}

		TCHAR			szStr[256]	= {NULL,};
		unsigned long	nRead		= sizeof(szStr);

		if( ::HttpQueryInfo(hRequest, HTTP_QUERY_STATUS_CODE, szStr, &nRead, NULL) )
		{
			//_log(_T("HTTP STATUS CODE:%s"), szStr);

			// 2013/11/22	Kim beomjin
			// ������ HTTP_STATUS_NOT_FOUND�� ��쿡�� ���� ���� ���з� ó���ϵ��� �ߴ�.
			// �׷��� HTTP_STATUS_SERVER_ERROR �� ���� ������ �������.
			// �Ͽ� HTTP_STATUS_OK �� ���� ��������� ���� ��쿡�� �������� ó���ϰ�
			// ������ �ڵ���� ��� ���з� �����ϵ��� �����Ѵ�.
			// 
			if( HTTP_STATUS_OK != _ttoi(szStr) )
				//if( HTTP_STATUS_NOT_FOUND == _ttoi(szStr) )
			{
				printf("error: url:[%ws] is not found.\n", pUrl);
				// FILE IS NOT FOUND
				break;
			}
		}
		else
		{
			//_log(_T("error: HttpQueryInfo() #1 failed. code = %d"), ::GetLastError());
			break;
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
			break;
		}

		DWORD	dwResponseBytes	= 0;
		bool	bRead		= true;
		bool	bLoop		= true;
		while( bLoop )
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
				bLoop	= false;
				break;
			}

			if( pCallback )
			{
				if( false == pCallback(pContext, lpPath, hFile, dwBytes, dwResponseBytes) )
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
		bRet	= true;
	}
	while( false );
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

#include <ShlObj.h>

bool	RequestUrl(IN LPCTSTR pUrl, OUT char ** pResponse, OUT DWORD * pdwResponseSize)
{
	bool	bRet	= false;

	TCHAR	szPath[LBUFSIZE],
			szFileName[LBUFSIZE];
	DWORD	dwSize	= sizeof(szPath);

	//	2016/08/31	KimPro
	//	���� Temp Directory ������ Y�� �Ǿ� �ְ� �� Y�� ���� ����� �ִ���. (KKS)
	//	�Ͽ� Temp�� ���ٰ� �̰� �ٲٱ�� �Ѵ�.
	if( GetAppTempPath( szPath, sizeof(szPath)) ) 
	{
		//::GetTempPath(sizeof(szPath)/sizeof(TCHAR), szPath);
		::GetTempFileName(szPath, NULL, 0, szFileName);
		//_log(_T("REQUEST URL[%s] to FILE[%s]"), pUrl, szFileName);

		if( RequestFile(pUrl, szFileName, &dwSize, NULL, NULL) )
		{
			if( pResponse && pdwResponseSize )
			{
				*pResponse	= (char *)GetFileContent(szFileName, pdwResponseSize);
				if( pResponse )
				{
					bRet	= true;
				}
			}
			else
			{
				bRet	= true;
			}
			::DeleteFile(szFileName);
		}
	}
	else
	{
		//_log(_T("error: SHGetSpecialFolderPath(CSIDL_APPDATA) failed. error=%d"), ::GetLastError());
	}
	return bRet;
}
YAGENT_COMMON_END