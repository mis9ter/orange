#pragma once

#include "mongoose.h"
#include "json/json.h"
#include "yagent.define.h"
#include "yagent.common.h"
#include "yagent.json.h"

#ifdef _WIN64	//_M_X64
#pragma comment(lib, "lz4lib.x64.lib")

#else
#pragma comment(lib, "lz4lib.win32.lib")

#endif

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Wininet.lib")

#define UPDATE_DIST_NAME	L".update"
#define DIST_LOGNAME		L"update.log"
class CDist
	:
	virtual	public	CAppLog
{
public:
	CDist() 
		:
		CAppLog(DIST_LOGNAME)
	{


	}
	~CDist() {


	}

	bool				Find(PCWSTR pRootPath, PCWSTR pPath, PCWSTR pCopyPath, Json::Value & doc) {

		WIN32_FIND_DATA		data;
		WCHAR				szPath[AGENT_PATH_SIZE] = L"";
		HANDLE				hFind;

		WCHAR				szCopyPath[AGENT_PATH_SIZE]	= L"";

		printf("%-32s %ws\n", __func__, pPath);
		StringCbPrintf(szPath, sizeof(szPath), L"%s\\*.*", pPath);
		hFind = FindFirstFile(szPath, &data);
		if (INVALID_HANDLE_VALUE == hFind)	return false;
		while (true) {
			if( pCopyPath )
				StringCbPrintf(szCopyPath, sizeof(szCopyPath), L"%s\\%s", pCopyPath, 
					std::wstring(pPath + (std::wstring)L"\\" + data.cFileName).c_str() +
					lstrlen(pRootPath) + 1);

			if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {
				if(L'.' != data.cFileName[0] ) {
					YAgent::MakeDirectory(szCopyPath);
					Find(pRootPath, std::wstring(pPath + (std::wstring)L"\\" + data.cFileName).c_str(), 
						pCopyPath? szCopyPath : NULL, doc);
				}
			}
			else if( _tcsicmp(data.cFileName, UPDATE_DIST_NAME) && data.nFileSizeLow ) {
				Json::Value		file;
				std::wstring	path	= pPath + (std::wstring)L"\\" + data.cFileName;
				std::wstring	apath	= pPath + (std::wstring)L"\\@" + data.cFileName;
				file["name"]	= __utf8(data.cFileName);
				file["path"]	= __utf8(path.c_str()) + lstrlen(pRootPath) + 1;
				file["apath"]	= __utf8(apath.c_str()) + lstrlen(pRootPath) + 1;
				file["size"]	= (int)data.nFileSizeLow;
				
				file["time"]["creation"]["high"] = (int)data.ftCreationTime.dwHighDateTime;
				file["time"]["creation"]["low"] = (int)data.ftCreationTime.dwLowDateTime;
				file["time"]["lastwrite"]["high"] = (int)data.ftLastWriteTime.dwHighDateTime;
				file["time"]["lastwrite"]["low"] = (int)data.ftLastWriteTime.dwLowDateTime;
				file["hash"] = FileHash(path.c_str());
				doc.append(file);

				if( pCopyPath ) {
					if (CopyFile(path.c_str(), szCopyPath, FALSE)) {
						printf("%-32s %ws => %ws\n", __func__, path.c_str(), szCopyPath);
					}
					else {
						CErrorMessage	err(GetLastError());

						printf("%-32s %ws => %ws failed.\n%ws\n", __func__, path.c_str(), szCopyPath,
							(PCWSTR)err);
					}
				}
			}
			if (FALSE == FindNextFile(hFind, &data))	break;
		}
		return true;
	}
	int					CreateProfile(PCWSTR pPath) {

		WCHAR		szPath[AGENT_PATH_SIZE] = L"";
		YAgent::GetModulePath(szPath, sizeof(szPath));
		SetCurrentDirectory(szPath);

		if (pPath && *pPath && PathIsRelative(pPath)) {
			GetFullPathName(pPath, _countof(szPath), szPath, NULL);
		}
		//printf("%-32s %ws\n", __func__, szPath);

		std::wstring	jsonpath = szPath + (std::wstring)L"\\" + UPDATE_DIST_NAME;
		JsonUtil::File2Json(jsonpath.c_str(), m_doc);
		JsonUtil::Json2String(m_doc, [&](std::string& doc) {

		});

		Json::Value&	doc = m_doc["file"];
		WCHAR			szCopyPath[AGENT_PATH_SIZE]	= L"";

		if (m_doc.isMember("path") && m_doc["path"].isString() ) {
			//	path가 존재하면 업데이트 대상 파일을 그곳으로 복사한다.
			StringCbCopy(szCopyPath, sizeof(szCopyPath), __utf16(m_doc["path"].asCString()));
		}
		doc.clear();
		int			nCount	= Find(szPath, szPath, szCopyPath[0]? szCopyPath : NULL, doc);

		JsonUtil::Json2File(m_doc, UPDATE_DIST_NAME);
		if (szCopyPath[0]) {
			StringCbCat(szCopyPath, sizeof(szCopyPath), L"\\");
			StringCbCat(szCopyPath, sizeof(szCopyPath), UPDATE_DIST_NAME);
			CopyFile(UPDATE_DIST_NAME, szCopyPath, FALSE);

		}
		return nCount;
	}
	bool				LoadProfile(PCWSTR pPath) {
		return JsonUtil::File2Json(pPath, m_doc);
	}
	Json::Value &		Profile() {
		return m_doc;
	}
	
	static	XXH64_hash_t		FileFastHash(PCWSTR pPath) {
#define	SAMPLE_THRESHOLD	(128 * 1024)
#define	SAMPLE_SIZE			(16 * 1024)		
		/*
			초간단 파일해시 너무 믿진 마.
			https://github.com/kalafut/py-imohash/blob/master/imohash/imohash.py
			이걸 간단히 VC++로 포팅함.
		*/
		XXH64_hash_t	v = 0;
		XXH64_state_t* p = XXH64_createState();
		DWORD			dwSize[2]	= { 0, };
		LARGE_INTEGER	lFileSize	= {0, };

		if (p) {
			HANDLE	hFile = INVALID_HANDLE_VALUE;
			__try {
				XXH64_reset(p, 0);
				hFile = ::CreateFile(pPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL, NULL);
				if (INVALID_HANDLE_VALUE == hFile)
				{
					//	해당 파일의 경로가 없는 경우 해시는 0인가요?
					__leave;
				}
				DWORD	dwBytes = 0;
				char	szBuf[SAMPLE_SIZE];
				if (GetFileSizeEx(hFile, &lFileSize)) {
					if (lFileSize.QuadPart < SAMPLE_THRESHOLD || SAMPLE_SIZE < 1) {
						//	다 읽는다.
						while (ReadFile(hFile, szBuf, sizeof(szBuf), &dwBytes, NULL) && dwBytes) {
							XXH64_update(p, szBuf, dwBytes);
						}
					}
					else {
						if (ReadFile(hFile, szBuf, sizeof(szBuf), &dwBytes, NULL) && dwBytes) {
							XXH64_update(p, szBuf, dwBytes);
						}
						LARGE_INTEGER	lFileSize2;
						lFileSize2.QuadPart	= lFileSize.QuadPart/2;
						if (SetFilePointerEx(hFile, lFileSize2, NULL, FILE_BEGIN)) {
							if (ReadFile(hFile, szBuf, sizeof(szBuf), &dwBytes, NULL) && dwBytes) {
								XXH64_update(p, szBuf, dwBytes);
							}
						}
						if (SetFilePointer(hFile, -1 * SAMPLE_SIZE, NULL, FILE_END)) {
							if (ReadFile(hFile, szBuf, sizeof(szBuf), &dwBytes, NULL) && dwBytes) {
								XXH64_update(p, szBuf, dwBytes);
							}
						}
					}					
				}
				XXH64_update(p, &lFileSize.QuadPart, sizeof(lFileSize.QuadPart));
				v = XXH64_digest(p);
			}
			__finally {
				if (INVALID_HANDLE_VALUE != hFile) {
					CloseHandle(hFile);
				}
			}
			XXH64_freeState(p);
		}
		return v;
	}
	static	XXH64_hash_t		FileHash(PCWSTR pPath) {
		XXH64_hash_t	v			= 0;
		XXH64_state_t*	p			= XXH64_createState();
		DWORD			dwSize[2]	= {0,};
		if (p) {
			HANDLE	hFile	= INVALID_HANDLE_VALUE;

			__try {
				XXH64_reset(p, 0);
				hFile = ::CreateFile(pPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL, NULL);
				if (INVALID_HANDLE_VALUE == hFile)
				{
					//	해당 파일의 경로가 없는 경우 해시는 0인가요?
					__leave;
				}
				DWORD	dwBytes = 0;
				char	szBuf[1024 * 64];
				while (ReadFile(hFile, szBuf, sizeof(szBuf), &dwBytes, NULL) && dwBytes ) {
					XXH64_update(p, szBuf, dwBytes);
				}
				dwSize[0]	= GetFileSize(hFile, &dwSize[1]);
				XXH64_update(p, dwSize, sizeof(dwSize));
				v = XXH64_digest(p);
			}
			__finally {
				if (INVALID_HANDLE_VALUE != hFile) {
					CloseHandle(hFile);
				}
			}
			XXH64_freeState(p);
		}
		return v;
	}
private:
	Json::Value			m_doc;
};



class CUpdate
	:
	public	CDist
{
public:
	CUpdate() {
		
	}
	~CUpdate() {


	}
	static		void	DeleteAFile(HMODULE hModule) {
		WCHAR	szPath[AGENT_PATH_SIZE]	= L"";
		PWSTR	pName;

		GetModuleFileName(hModule, szPath, _countof(szPath));
		pName	= _tcsrchr(szPath, L'\\');
		if( pName )	{
			*pName++	= NULL;
		}
		else pName	= szPath;

		if( L'@' != *pName ) {
			std::wstring	path	= szPath;
			path	+= L"\\";
			path	+= L"@";
			path	+= pName;
			
			DeleteFile(path.c_str());
		}	
	}
	static	bool	IsFileReadable(PCWSTR pPath)
	{
		HANDLE		hFile = INVALID_HANDLE_VALUE;

		hFile = CreateFile(pPath,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL);
		if(hFile == INVALID_HANDLE_VALUE)
		{
			return false;
		}
		CloseHandle(hFile);
		return true;
	}
	static	bool	IsFileWrittable(PCWSTR pPath)
	{
		HANDLE		hFile = INVALID_HANDLE_VALUE;

		hFile = CreateFile(pPath,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL);
		if(hFile == INVALID_HANDLE_VALUE)
		{
			return false;
		}
		CloseHandle(hFile);
		return true;
	}
	int					UpdateFile(bool bUpdate) {
		Json::Value	&file	= Profile()["file"];

		WCHAR	szPath[AGENT_PATH_SIZE]			= L"";
		WCHAR	szTempPath[AGENT_PATH_SIZE]		= L"";

		int		nUpdateRequired			= 0;
		YAgent::GetModulePath(szPath, sizeof(szPath));
		StringCbPrintf(szTempPath, sizeof(szTempPath), L"%s\\temp", szPath);
		if( PathIsDirectory(szTempPath) )	YAgent::DeleteDirectoryFiles(szTempPath);
		else	YAgent::MakeDirectory(szTempPath);

		for( auto & t : file ) {
			try {
				Json::Value		&update	= t["update"];
				std::wstring	path	= szPath + (std::wstring)L"\\" + __utf16(t["path"].asCString());				
				std::wstring	url		= __utf16(Profile()["url"].asString());
				std::wstring	fileurl	= url + L"/" + __utf16(t["path"].asCString());

				update	= false;

				ULONG	nThisFileSize	= YAgent::GetFileSize(path.c_str(), NULL);
				ULONG	nNextFileSize	= t["size"].asInt();
				printf("[%ws]\n", __utf16(t["path"].asCString()));
				if( nThisFileSize != nNextFileSize ) {
					printf("  size: %d -> %d\n", nThisFileSize, nNextFileSize);
					update	= true;
				}
				else {
					XXH64_hash_t	hash	= FileHash(path.c_str());					
					if( hash != t["hash"].asLargestUInt() ) {
						printf("  hash: %p -> %p\n", (PVOID)hash, (PVOID)t["hash"].asLargestUInt());
						update	= true;
					}
					else {
						printf("  hash: %p\n", (PVOID)t["hash"].asLargestUInt());
					}
				}
				if( update.asBool() ) {
					WCHAR	szAPath[AGENT_PATH_SIZE]	= L"";
					GetTempFileName(szTempPath, __utf16(t["name"].asCString()), GetTickCount(), szAPath);

					printf("  downloading: %ws\n", szAPath);
					std::wstring	apath	= szAPath;

					if( bUpdate ) {
						bool	bMoved	= false;
						if( PathFileExists(path.c_str()) && !IsFileWrittable(path.c_str()) ) {
							//	파일이 존재하고 쓰기 불가하면 이동
							if( MoveFileEx(path.c_str(), apath.c_str(), MOVEFILE_REPLACE_EXISTING) ) {
								//	이동됨
								bMoved	= true;
							}
							else {
								CErrorMessage	err(GetLastError());
								printf("  %-32s(1) %d %s\n", __func__, (DWORD)err, (PCSTR)err);
							}
						}
						FILETIME	creation, lastwrite;

						creation.dwHighDateTime		= t["time"]["creation"]["high"].asInt();
						creation.dwLowDateTime		= t["time"]["creation"]["low"].asInt();
						lastwrite.dwHighDateTime	= t["time"]["lastwrite"]["high"].asInt();
						lastwrite.dwLowDateTime		= t["time"]["lastwrite"]["low"].asInt();	

						bool	bDownload	= YAgent::RequestFile(fileurl.c_str(), path.c_str(), NULL, this,
									[&](PVOID pContext, PCWSTR pPath, HANDLE hFile, DWORD dwTotalBytes, DWORD dwCurrentBytes)->bool {
										if( dwTotalBytes == dwCurrentBytes ) {
											printf("  downloaded: %d bytes\n", dwCurrentBytes);
											SetFileTime(hFile, &creation, NULL, &lastwrite);										
										}
										return true;
						});
						if( bDownload ) {
							if( bMoved ) {
								MoveFileEx(apath.c_str(), NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
							}
							else {

							}
						}
						else {
							if( bMoved ) {
								MoveFileEx(apath.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING);	
							}
							else {


							}
						}
						printf("  %ws -> %ws [%d]\n", fileurl.c_str(), path.c_str(), bDownload);
						nUpdateRequired++;
					}
					else {
						nUpdateRequired++;
						printf("  update required.\n");
					}
					printf("\n");
				}	
				else {
					printf("  no update\n");
				}
			}
			catch( std::exception & e) {
				printf("%-32s %s\n", __func__, e.what());
			}
		}
		return nUpdateRequired;
	}
	int					Update() {
		int		nCount = 0;

		std::wstring	url;

		try {
			LoadProfile(UPDATE_DIST_NAME);
			if( Profile().isMember("url") && Profile()["url"].isString() )
				url	= __utf16(Profile()["url"].asCString());
			
			if( url.empty() )
				url	= L"http://orangeworks.org/orange";

			if( DownloadProfile((url + L"/" UPDATE_DIST_NAME).c_str(), UPDATE_DIST_NAME) ) {

			}
			if( false == LoadProfile(UPDATE_DIST_NAME) )	return 0;		

			Json::Value	&action	= Profile()["action"];
			Json::Value	&pre	= Profile()["action"]["pre"];
			Json::Value	&post	= Profile()["action"]["post"];
			
			int	nUpdateRequired	= UpdateFile(false);
			if( nUpdateRequired ) 
			{

				if( !pre.empty() ) {
					for( auto & t : pre ) {
						printf("%s\n", t.asCString());
						HANDLE	p	= YAgent::Run(__utf16(t.asCString()), NULL);
						printf("%p\n", p);
						if( p ) {
							WaitForSingleObject(p, 30 * 1000);
							CloseHandle(p);
						}
					}
				}

				UpdateFile(true);

				if( !post.empty() ) {
					for( auto & t : post ) {
						printf("%s\n", t.asCString());
						HANDLE	p	= YAgent::Run(__utf16(t.asCString()), NULL);
						printf("%p\n", p);
						if( p ) {
							WaitForSingleObject(p, 30 * 1000);
							CloseHandle(p);
						}
					}
				}			
			}

		}
		catch( std::exception & e) {
			printf("%-32s %s\n", __func__, e.what());
		}
		return nCount;
	}

private:

	bool				DownloadProfile(PCWSTR pUrl, PCWSTR pFilePath) {
		bool		bRet = false;
	
		bRet	= YAgent::RequestFile(
			pUrl, pFilePath, NULL, this, NULL);

		printf("download:%ws[%d]\n", pUrl, bRet);
		return bRet;
	}

	//	https://github.com/cesanta/mongoose/blob/master/test/unit_test.c

	static void f3(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
		int *ok = (int *) fn_data;
		// LOG(LL_INFO, ("%d", ev));
		if (ev == MG_EV_CONNECT) {
			// c->is_hexdumping = 1;
			printf("connected\n");
			mg_printf(c, "GET / HTTP/1.0\r\nHost: %s\r\n\r\n",
				c->peer.is_ip6 ? "orange" : "orange");
		} else if (ev == MG_EV_HTTP_MSG) {
			struct mg_http_message *hm = (struct mg_http_message *) ev_data;
			printf("-->[%.*s]\n", (int) hm->message.len, hm->message.ptr);
			mg_http_serve_file(c, hm, ".update", "text/plain", "");

			// LOG(LL_INFO, ("-->[%.*s]", (int) hm->message.len, hm->message.ptr));
			// ASSERT(mg_vcmp(&hm->method, "HTTP/1.1") == 0);
			// ASSERT(mg_vcmp(&hm->uri, "301") == 0);
			*ok = atoi(hm->uri.ptr);
		} else if (ev == MG_EV_CLOSE) {
			if (*ok == 0) *ok = 888;
		} else if (ev == MG_EV_ERROR) {
			if (*ok == 0) *ok = 777;
		}
	}
	bool				DownloadProfile2() {
		bool		bRet = false;

		struct mg_mgr			mgr;
		struct mg_connection	*c;
		int i, ok = 0;

		mg_mgr_init(&mgr);
		c	= mg_http_connect(&mgr, "http://download.orangeworks.org", f3, &ok);
		if( c ) {
			
		
		}		
		for (i = 0; i < 500 && ok <= 0; i++) mg_mgr_poll(&mgr, 10);
		c->is_closing = 1;
		mg_mgr_poll(&mgr, 0);
		mg_mgr_free(&mgr);
		return bRet;
	}

};
