#pragma once
#include "framework.h"
#include "crc64.h"

typedef enum StringType 
{
	StringProcPath,
	StringProcDevicePath,
	StringProcName,
	StringCommand,
	StringFileDevicePath,
	StringFilePath,
	StringFileName,
	StringRegistryKey,
	StringRegistryValue,
	StringModulePath,
	StringModuleName,

} StringType;

typedef struct _STRING {
	_STRING(PCWSTR pStr) 
	{
		if( pStr && *pStr )	value	= pStr;
		SUID		= 0;
		nLastTick	= GetTickCount64();
		nCount		= 1;
		bSave		= false;
	}
	std::wstring		value;
	STRUID				SUID;
	volatile LONG64		nLastTick;
	UINT64				nCreateTime;
	StringType			type;
	volatile	ULONG	nCount;
	bool				bSave;
} STRING, *PSTRING;

struct c_str_icompare
{
	bool operator()(LPCTSTR s1, LPCTSTR s2)const{return _tcsicmp(s1,s2)<0;}
};
struct c_str_compare
{
	bool operator()(LPCTSTR s1, LPCTSTR s2)const{return _tcscmp(s1,s2)<0;}
};
/*
struct c_str_xcompare
{
	bool operator()(LPCTSTR s1, LPCTSTR s2)const{return xcommon::CompareBeginningString(s1,s2)<0;}
};

struct c_str_xcompareA
{
	bool operator()(LPCSTR s1, LPCSTR s2)const{return xcommon::CompareBeginningStringA(s1,s2)<0;}
};
*/
typedef std::shared_ptr<STRING>						StrPtr;
typedef std::map<PCWSTR, StrPtr, c_str_icompare>	StrMap;
typedef std::map<STRUID, StrPtr>					StrUIDMap;

class CStringTable
	:
	public	CCRC64,
	virtual	public	CAppLog,
	virtual	public	CStmt
{
public:
	CStringTable() 
		:
		m_pDB(NULL),
		m_szNull(L""),
		m_log(L"string.log")
	{
		m_log.Log(NULL);
	}
	~CStringTable() {
		m_log.Log(__func__);
	}
	virtual	INotifyCenter *	NotifyCenter() = NULL;
	void			Create() {
		NotifyCenter()->RegisterNotifyCallback(__func__, NOTIFY_TYPE_AGENT, NOTIFY_EVENT_PERIODIC, 10, 
							this, PeriodicCallback);
	}	
	void			Destroy() {
		Flush(true);
	}
	STRUID			GetStrUID(IN StringType type, IN PCWSTR pStr) {
		STRUID	SUID	= 0;
		Lock(NULL, [&](PVOID pContext) {
			auto	t	= m_str.find(pStr);
			if( m_str.end() == t ) {
				SUID	= GetStringCRC64(pStr);		
				try {
					StrPtr	ptr	= std::make_shared<STRING>(pStr);
					ptr->SUID	= SUID;
					ptr->type	= type;
					std::pair<StrMap::iterator, bool>	ret	= m_str.insert(std::pair(ptr->value.c_str(), ptr));
					if( ret.second ) {
						m_uid.insert(std::pair(SUID, ptr));					
					}
				}
				catch(std::exception &e) {
					UNREFERENCED_PARAMETER(e);			
				}
			}
			else {
				SUID	= t->second->SUID;
				t->second->nCount++;
				t->second->nLastTick	= GetTickCount64();
			}
		});
		return SUID;
	}
	PCWSTR			GetString(STRUID SUID, std::wstring & str) {
		auto	t	= m_uid.find(SUID);
		DWORD64	dwTicks	= GetTickCount64();

		Lock(NULL, [&](PVOID pContext) {
			if( m_uid.end() == t ) {
				//	메모리에 없는 경우 DB에서 읽습니다.
				STRING	s(NULL);
				if( Select1(SUID, &s) ) {			
					str	= s.value;

					StrPtr	ptr	= std::make_shared<STRING>(str.c_str());
					ptr->SUID	= SUID;
					ptr->type	= s.type;
					ptr->nCount++;

					std::pair<StrMap::iterator, bool>	ret	= m_str.insert(std::pair(ptr->value.c_str(), ptr));
					if( ret.second ) {
						m_uid.insert(std::pair(SUID, ptr));					
					}
					m_log.Log("%s %I64d %ws[%d]", "Select1", SUID, s.value.c_str(), s.value.length());
				}
				else {
					str	= std::to_wstring(SUID);
				}				
			}	
			else	{
				str	= t->second->value.c_str();
				t->second->nLastTick	= dwTicks;
				//m_log.Log("%s %I64d %ws[%d]", "MEM", SUID, str.c_str(), str.length());
			}
		});
		return str.c_str();
	}
	void			Flush(bool bAll = false) {
		DWORD64	dwTick		= GetTickCount64();
		ULONG	nDeleted	= 0;
		bool	bDelete	= false;

		DWORD	dwUpsert	= 0;

		//Db()->Begin(__func__);
		Begin();
		Lock(NULL, [&](PVOID pContext) {
			for( auto t = m_str.begin()  ; t != m_str.end() ; ) {			
				bDelete	= false;
				if(  bAll )	{
					Upsert(t->second.get());
					dwUpsert++;
					bDelete	= true;
				}
				else {
					bool	bOld	= IsOld(t->second.get());
					if( false == t->second->bSave || bOld ) {
						if( Upsert(t->second.get()) ) {
							dwUpsert++;
							t->second->bSave = true;
						}
						bDelete	= bOld;
					}
				}
				if( bDelete ) {
					auto	d	= t++;
					m_log.Log("%s %.2f %ws(%I64d)", "Flush", (GetTickCount64()-d->second->nLastTick)/1000.0, 
						d->second->value.c_str(), d->second->SUID);
					m_uid.erase(d->second->SUID);
					m_str.erase(d);
					nDeleted++;					
				}
				else 
					t++;
			}		
			if( bAll ) {
				m_uid.clear();
				m_str.clear();
			}
			m_log.Log("%-32s T:%d UPSERT:%d D:%d", __func__, m_str.size(), dwUpsert, nDeleted);
		});	
		Commit();
	}
	void			Lock(PVOID pContext, std::function<void (PVOID)> pCallback) {
		m_lock.Lock(pContext, pCallback);
	}
	STRUID			GetStringCRC64(PCWSTR pValue) {
		if( NULL == pValue )	return 0;
		return GetCrc64((PVOID)pValue, (DWORD)(wcslen(pValue) * sizeof(WCHAR)));	
	}
private:
	CAppLog		m_log;
	CDB			*m_pDB;
	CLock		m_lock;
	StrUIDMap	m_uid;
	StrMap		m_str;
	WCHAR		m_szNull[1];


	bool			IsOld(PSTRING p, DWORD64 dwTick = 0) {
		if( 0 == dwTick )	dwTick = GetTickCount64();
		if( (dwTick - p->nLastTick) >= 60 * 1000 ) {
			//m_log.Log("%04d %ws", p->nCount, p->value.c_str());
			return true;
		}
		return false;
	}
	struct {
		sqlite3_stmt	*pInsert;
		sqlite3_stmt	*pSelect1;
		sqlite3_stmt	*pSelect2;
		sqlite3_stmt	*pUpdate;
		sqlite3_stmt	*pIsExist;
	}	m_stmt;
	bool			IsExisting(IN STRUID SUID) {
		int			nCount	= 0;
		Json::Value	req;
		Json::Value	&bind	= req["bind"];
		Json::Value	res;

		req["name"]	= "string.isExisting";
		CStmt::BindUInt64(	bind[0],	SUID);
		nCount	= Query(req, res, false, [&](int nErrorCode, const char * pErrorMessage) {
			m_log.Log("[%d] %s", nErrorCode, pErrorMessage);
		});
		if( nCount ) {
			try {
				nCount	= res["row"][0][0].asInt();
			}
			catch( std::exception & e) {
				m_log.Log("%-32s %s", __func__, e.what());
			}
		}
		return nCount ? true : false;
	}
	bool			Update(IN PSTRING p) {
		int			nCount	= 0;
		Json::Value	req;
		Json::Value	&bind	= req["bind"];
		Json::Value	res;

		req["name"]	= "string.update";
		CStmt::BindUInt64(	bind[0],	p->SUID);
		nCount	= Query(req, res, false, [&](int nErrorCode, const char * pErrorMessage) {
			m_log.Log("[%d] %s", nErrorCode, pErrorMessage);
		});
		res["count"]	= nCount;
		if( nCount ) {
			try {
				nCount	= res["row"][0][0].asInt();
			}
			catch( std::exception & e) {
				m_log.Log("%-32s %s", __func__, e.what());
			}
		}
		return nCount ? true : false;	
	}
	bool			Upsert(IN PSTRING p) {
		if( IsExisting(p->SUID)) {
			return Update(p);
		}	
		return Insert(p);
	}
	bool			Commit() {

		int			nCount	= 0;
		Json::Value	req;
		Json::Value	&bind	= req["bind"];
		Json::Value	res;

		req["name"]	= "string.commit";
		nCount	= Query(req, res, false, [&](int nErrorCode, const char * pErrorMessage) {
			m_log.Log("[%d] %s", nErrorCode, pErrorMessage);
			m_log.Log("---- req ----");
			JsonUtil::Json2String(req, [&](std::string &str) {
				m_log.Log(str.c_str());
			});
		});
		return true;
	}
	bool			Begin() {

		int			nCount	= 0;
		Json::Value	req;
		Json::Value	&bind	= req["bind"];
		Json::Value	res;

		req["name"]	= "string.begin";
		nCount	= Query(req, res, false, [&](int nErrorCode, const char * pErrorMessage) {
			m_log.Log("[%d] %s", nErrorCode, pErrorMessage);
			m_log.Log("---- req ----");
			JsonUtil::Json2String(req, [&](std::string &str) {
				m_log.Log(str.c_str());
			});
		});
		return true;
	}
	bool			Select1(IN STRUID SUID, OUT PSTRING p) {

		int			nCount	= 0;
		Json::Value	req;
		Json::Value	&bind	= req["bind"];
		Json::Value	res;

		req["name"]	= "string.select1";
		CStmt::BindUInt64(	bind[0],	SUID);
		nCount	= Query(req, res, false, [&](int nErrorCode, const char * pErrorMessage) {
			m_log.Log("[%d] %s", nErrorCode, pErrorMessage);
			m_log.Log("---- req ----");
			JsonUtil::Json2String(req, [&](std::string &str) {
				m_log.Log(str.c_str());
			});
		});
		res["count"]	= nCount;
		if( nCount ) {
			try {
				//m_log.Log("---- res ----");
				//JsonUtil::Json2String(res, [&](std::string &str) {
				//	m_log.Log(str.c_str());
				//});
				p->SUID		= SUID;
				p->value	= __utf16(res["row"][0][1].asCString());
				p->nCount	= res["row"][0][2].asUInt();
				p->nLastTick= GetTickCount64();
			}
			catch( std::exception & e) {
				m_log.Log("%-32s %s", __func__, e.what());
				m_log.Log("---- res ----");
				JsonUtil::Json2String(res, [&](std::string &str) {
					m_log.Log(str.c_str());
				});
			}
		}
		return nCount ? true : false;	
	}
	bool			Select2(IN PCWSTR pStr, OUT PSTRING p) {

		int			nCount	= 0;
		Json::Value	req;
		Json::Value	&bind	= req["bind"];
		Json::Value	res;

		req["name"]	= "string.select2";
		CStmt::BindString(	bind[0],	pStr);
		nCount	= Query(req, res, false, [&](int nErrorCode, const char * pErrorMessage) {
			m_log.Log("[%d] %s", nErrorCode, pErrorMessage);
		});
		res["count"]	= nCount;
		if( nCount ) {
			try {
				nCount	= res["row"][0][0].asInt();
			}
			catch( std::exception & e) {
				m_log.Log("%-32s %s", __func__, e.what());
			}
		}
		JsonUtil::Json2String(res, [&](std::string & str) {
			m_log.Log(str.c_str());
		});
		return nCount ? true : false;	
		/*
		bool			bRet	= false;
		sqlite3_stmt	*pStmt	= NULL;
		if (pStmt) {
			int		nBind	= 0;
			sqlite3_bind_text16(pStmt,	++nBind, pStr, -1, SQLITE_STATIC);
			if (SQLITE_ROW == sqlite3_step(pStmt))	{
				int			nCol	= 0;
				PCWSTR		pValue	= NULL;
				uint64_t	nCount	= 0;

				p->SUID		= sqlite3_column_int64(pStmt, nCol++);
				pValue		= (PCWSTR)sqlite3_column_text16(pStmt, nCol++);
				p->nCount	= (DWORD)sqlite3_column_int64(pStmt, nCol++);
				p->value	= pValue;
				bRet = true;
			}
			else {
				m_log.Log("%-32s STR:%ws %s", __func__, pStr, sqlite3_errmsg(Db()->Handle()));
			}
			sqlite3_reset(pStmt);
		} 
		return bRet;
		*/
	}
	/*
	bool			Select1_OLD(IN STRUID SUID, OUT PSTRING p) {

		bool			bRet	= false;
		sqlite3_stmt	*pStmt	= NULL;
		int				nRet	= 0;
		if (pStmt) {
			int		nBind	= 0;
			sqlite3_bind_int64(pStmt,	++nBind, SUID);
			if (SQLITE_ROW == (nRet = sqlite3_step(pStmt)))	{
				int			nCol	= 0;
				PCWSTR		pValue	= NULL;
				uint64_t	nCount	= 0;

				p->SUID		= sqlite3_column_int64(pStmt, nCol++);
				pValue		= (PCWSTR)sqlite3_column_text16(pStmt, nCol++);
				p->nCount	= (DWORD)sqlite3_column_int64(pStmt, nCol++);
				p->value	= pValue? pValue : L"";
				bRet = true;
			}
			else {
				m_log.Log("%-32s RET:%d SUID:%s %s", __func__, nRet, std::to_string(SUID).c_str(), sqlite3_errmsg(Db()->Handle()));
			}
			sqlite3_reset(pStmt);
		} 
		return bRet;
	}
	bool			Select2_OLD(IN PCWSTR pStr, OUT PSTRING p) {
	
		bool			bRet	= false;
		sqlite3_stmt	*pStmt	= NULL;
		if (pStmt) {
			int		nBind	= 0;
			sqlite3_bind_text16(pStmt,	++nBind, pStr, -1, SQLITE_STATIC);
			if (SQLITE_ROW == sqlite3_step(pStmt))	{
				int			nCol	= 0;
				PCWSTR		pValue	= NULL;
				uint64_t	nCount	= 0;

				p->SUID		= sqlite3_column_int64(pStmt, nCol++);
				pValue		= (PCWSTR)sqlite3_column_text16(pStmt, nCol++);
				p->nCount	= (DWORD)sqlite3_column_int64(pStmt, nCol++);
				p->value	= pValue;
				bRet = true;
			}
			else {
				m_log.Log("%-32s STR:%ws %s", __func__, pStr, sqlite3_errmsg(Db()->Handle()));
			}
			sqlite3_reset(pStmt);
		} 
		return bRet;
	}
	*/
	bool			Insert(IN PSTRING p) {
		int			nCount	= 0;
		Json::Value	req;
		Json::Value	&bind	= req["bind"];
		Json::Value	res;

		req["name"]	= "string.insert";
		CStmt::BindUInt64(	bind[0],	p->SUID);
		CStmt::BindString(	bind[1],	p->value.c_str());
		CStmt::BindUInt64(	bind[2],	p->nCount);
		CStmt::BindInt(		bind[3],	p->type);	
		nCount	= Query(req, res, false, [&](int nErrorCode, const char * pErrorMessage) {
			m_log.Log("[%d] %s", nErrorCode, pErrorMessage);
		});
		res["count"]	= nCount;
		if( nCount ) {
			try {
				nCount	= res["row"][0][0].asInt();
			}
			catch( std::exception & e) {
				m_log.Log("%-32s %s", __func__, e.what());
			}
		}
		return nCount ? true : false;	
	}
	static	void	PeriodicCallback(
		WORD wType, WORD wEvent, PVOID pData, ULONG_PTR nDataSize, PVOID pContext
	) {
		CStringTable	*pClass	= (CStringTable *)pContext;
		static	DWORD	dwCount;

		pClass->m_log.Log(__func__);
		pClass->Flush();
	}
};
