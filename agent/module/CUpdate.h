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

class CDist
{
public:
	CDist() {


	}
	~CDist() {


	}

	bool				Find(PCWSTR pRootPath, PCWSTR pPath, Json::Value & doc) {

		WIN32_FIND_DATA		data;
		WCHAR				szPath[AGENT_PATH_SIZE] = L"";
		HANDLE				hFind;

		printf("%-32s %ws\n", __func__, pPath);
		StringCbPrintf(szPath, sizeof(szPath), L"%s\\*.*", pPath);
		hFind = FindFirstFile(szPath, &data);
		if (INVALID_HANDLE_VALUE == hFind)	return false;
		while (true) {
			if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {
				if(L'.' != data.cFileName[0] )
					Find(pRootPath, std::wstring(pPath + (std::wstring)L"\\" + data.cFileName).c_str(), doc);
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

		Json::Value& doc = m_doc["file"];
		doc.clear();
		int			nCount	= Find(szPath, szPath, doc);

		JsonUtil::Json2File(m_doc, L".update");
		return nCount;
	}
	bool				LoadProfile(PCWSTR pPath) {
		return JsonUtil::File2Json(pPath, m_doc);
	}
	Json::Value &		Profile() {
		return m_doc;
	}
	XXH64_hash_t		FileHash(PCWSTR pPath) {
		XXH64_hash_t	v	= 0;
		XXH64_state_t*	p	= XXH64_createState();
		if (p) {
			HANDLE	hFile	= INVALID_HANDLE_VALUE;

			__try {
				XXH64_reset(p, 0);
				hFile = ::CreateFile(pPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL, NULL);
				if (INVALID_HANDLE_VALUE == hFile)
				{
					__leave;
				}
				DWORD	dwBytes = 0;
				char	szBuf[4096];
				while (ReadFile(hFile, szBuf, sizeof(szBuf), &dwBytes, NULL) && dwBytes ) {
					XXH64_update(p, szBuf, dwBytes);
				}
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

				printf("-- %ws\n", __utf16(t["path"].asCString()));
				if( YAgent::GetFileSize(path.c_str(), NULL) != t["size"].asInt() ) {
					update	= true;
				}
				else {
					XXH64_hash_t	hash	= FileHash(path.c_str());
					printf("  %p -> %p\n", hash, t["hash"].asLargestUInt());
					if( hash != t["hash"].asLargestUInt() ) {
						update	= true;
					}
				}
				printf("\n");
				if( update.asBool() ) {
					WCHAR	szAPath[AGENT_PATH_SIZE]	= L"";
					GetTempFileName(szTempPath, __utf16(t["name"].asCString()), GetTickCount(), szAPath);

					printf("%ws\n", szAPath);
					std::wstring	apath	= szAPath;

					if( bUpdate ) {
						if( MoveFileEx(path.c_str(), apath.c_str(), MOVEFILE_REPLACE_EXISTING) ) {
							FILETIME	creation, lastwrite;

							creation.dwHighDateTime		= t["time"]["creation"]["high"].asInt();
							creation.dwLowDateTime		= t["time"]["creation"]["low"].asInt();
							lastwrite.dwHighDateTime	= t["time"]["lastwrite"]["high"].asInt();
							lastwrite.dwLowDateTime		= t["time"]["lastwrite"]["low"].asInt();	

							bool	bRet	= YAgent::RequestFile(fileurl.c_str(), path.c_str(), NULL, this,
										[&](PVOID pContext, PCWSTR pPath, HANDLE hFile, DWORD dwTotalBytes, DWORD dwCurrentBytes)->bool {
											if( dwTotalBytes == dwCurrentBytes ) {
												printf("%-32s %d / %d\n", __func__, dwCurrentBytes, dwTotalBytes);
												SetFileTime(hFile, &creation, NULL, &lastwrite);										
											}
											return true;
										});
							if( bRet ) {
								if( DeleteFile(apath.c_str())) {
								
								}	
								else {
									MoveFileEx(apath.c_str(), NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
								}
							}
							else {
								MoveFileEx(apath.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING);							
							}
							printf("%ws -> %ws [%d]", fileurl.c_str(), path.c_str(), bRet);
							nUpdateRequired++;
							printf("  updated.\n");					
						}
						else {
							CErrorMessage	err(GetLastError());
							printf("%-32s %d %s\n", __func__, (DWORD)err, (PCSTR)err);
						}
					}
					else {
						nUpdateRequired++;
						printf("  update required.\n");
					}
				}	
				else {
					printf("not update\n");
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
				url	= L"http://download.orangeworks.org";

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
