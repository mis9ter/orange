#pragma once

/*
	table.json에 정의된 형식을 자동으로 sqlite table로 생성/관리해준다.
	개발자는 sqlite table에 상관없이 class로 처리한다. 

	1. 기본적으로 row들은 memory에서 처리하고 싶다. 
	2. 필요에 따라 row들을 file로 떨어뜨리고 싶다. 


	이걸 꼭 sqlite를 써야할 이유가 있을까?
	그냥 map으로 구현해버려??

	프로그램 종료시 파일로 save하는 경우를 생각하면 sqlite가 맞을 것도 같다. 




	CDbTable 
	1개의 db를 가진 클래스


	CDbTableFactory
	1. CDbTable.json 생성/읽기
	2. 1에 따라서 필요한만큼 CDbTable 생성
	3. 외부로 CDbTable 제공

	결과적으로 외부 님들은 sqlite에 대해선 모르게 된다. 

	여러개의 db를 조인해야 하면 어떻게 하지???
*/
#include <list>

#include "json/json.h"
#include "CAppLog.h"
#include "CDB.h"

#define	CDBTABLE_LOG_NAME			L"cdbtable"
#define CDB_DEFAULT_TRANSACTION		300
typedef enum {
	ColumnTypeINTEGER,
	ColumnTypeINT32,
	ColumnTypeINT64,
	ColumnTypeUINT32,
	ColumnTypeUINT64,
	ColumnTypeNVARCHAR2,
	ColumnTypeTEXT,
	ColumnTypeDATA,
} CDbDataType;

typedef enum {
	StorageFile,
	StorageMemory,
} CDbStorageType;

#define	CDB_COLUMN_ALL		0xffffffff
#define	CDB_COLUMN_KEY		0x00000001
#define	CDB_COLUMN_INDEX	0x00000002
#define	CDB_COLUMN_DATA		0x00000004
#define CDB_COLUMN_COUNT	0x00000008
#define CDB_COLUMN_SUM		0x00000010
#define CDB_COLUMN_UPDATE	0x00000011

typedef DWORD				CDbColumnType;

typedef struct {
	std::string		name;
	std::string		type;
	CDbDataType		dType;
	CDbColumnType	cType;
	std::string		option;
	std::string		_default;
} CDbColumn;

typedef std::shared_ptr<CDbColumn>			ColumnPtr;
typedef std::map<std::string, ColumnPtr>	ColumnMap;

class CDbClass
	:
	public	CDB,
	virtual	public	CAppLog
{
public:
	CDbClass
	(
		PCWSTR			pLogName,		//	CDbClass별로 1개의 로그를 생성한다.
		std::string		db,				//	db file 
		std::string		table,			//	table name
		Json::Value		&doc
	) 
		:
		CAppLog(pLogName),
		m_dwOP(0),
		m_bBegin(false)
	{
		try {
			Log(__func__);
			ZeroMemory(&m_stmt, sizeof(m_stmt));
			m_config.db		= db;
			m_config.table	= table;

			YAgent::GetDataFolder(m_path.root);
			m_path.odb	= m_path.root + L"\\" + CDBTABLE_LOG_NAME + L"." + __utf16(m_config.db) + L".odb";
			m_path.cdb	= m_path.root + L"\\" + CDBTABLE_LOG_NAME + L"." + __utf16(m_config.db) + L".cdb";
			m_path.mdb	= m_path.root + L"\\" + CDBTABLE_LOG_NAME + L"." + __utf16(m_config.db) + L".mdb";

			
			m_config.storageType	= StorageFile;
			if (doc.isMember("storage") && doc["storage"].isString() ) {
				if (!doc["storage"].asString().compare("memory"))
					m_config.storageType = StorageMemory;
			}
			if (doc.isMember("persistent") && doc["persitent"].isBool()) {
				m_config.bPersistent	= doc["persistent"].asBool();
			}
			else {
				m_config.bPersistent	= false;
			}

			m_config.dwTransaction	= (DWORD)JsonUtil::GetInt(doc, "transaction", 
				CDB_DEFAULT_TRANSACTION, 1, 100000,
				[&](const Json::Value& doc, PCSTR pName, PCSTR pErrMsg) {

			});

			CreateDDL(table, doc, m_config.doc);
			Json::Value		&Path	= m_config.doc["path"];

			Path["odb"]	= __utf8(m_path.odb);
			Path["cdb"]	= __utf8(m_path.cdb);
			Path["mdb"]	= __utf8(m_path.mdb);

			if (doc.isMember("delete") && !doc["delete"].empty()) {
				m_config.doc["delete"]	= doc["delete"];
			}
			if (PathFileExists(m_path.cdb.c_str())) {

			}
			else {

			}
			JsonUtil::Json2String(m_config.doc, [&](std::string& str) {
				Log("%-32s %s", __func__, str.c_str());

			});
		}
		catch( std::exception & e) {
			Log("%-32s %s", __func__, e.what());
		}	
	}
	virtual	~CDbClass() {

	}

	Json::Value& GetJson(IN PCSTR pCause, IN Json::Value& doc, IN std::string name) {
		try {
			return doc[name];
		}
		catch (std::exception& e) {
			Log("%-32s %s:%s", pCause, name.c_str(), e.what());
			throw e;
		}
	}
	int			SetColumn(ColumnMap& m, std::string& ddl,
		int nCountOfColumn, Json::Value& doc, DWORD dwType, bool bTail = false) {
		int				nCount = 0;
		std::string		str, strall;

		for (auto& t : doc.getMemberNames()) {
			try {
				ColumnPtr	ptr = std::make_shared<CDbColumn>();
				ptr->name = t;
				ptr->cType = dwType;
				str.clear();

				if (doc[t].isMember("type")) {
					ptr->type = doc[t]["type"].asString();
					if (!ptr->type.compare("INTEGER")) {
						int		nSize = doc[t]["size"] ? doc[t]["size"].asInt() : 32;
						bool	bSigned = doc[t].isMember("signed") ? doc[t]["signed"].asBool() : true;

						if (64 == nSize) {
							ptr->dType = bSigned ? ColumnTypeINT64 : ColumnTypeUINT64;
						}
						else
							ptr->dType = bSigned ? ColumnTypeINT32 : ColumnTypeUINT32;
					}
					else if (!ptr->type.compare("NVARCHAR2")) {
						ptr->dType = ColumnTypeNVARCHAR2;
					}
					else if (!ptr->type.compare("DATA")) {
						ptr->dType = ColumnTypeDATA;
						//	정의문에는 DATA로 적혀있으나, sqlite에선 TEXT를 사용한다.
						ptr->type = "TEXT";
					}
					else if (!ptr->type.compare("TEXT")) {
						ptr->dType = ColumnTypeTEXT;
					}
					else {
						Log("%-32s %s %s", __func__, "unknown type", ptr->type.c_str());
					}
					str += "  [" + t + "] " + ptr->type;
					if (dwType & CDB_COLUMN_KEY)	str += " PRIMARY KEY ";
				}
				if (doc[t].isMember("default")) {
					ptr->_default = " " + doc[t]["default"].asString();
					str += " DEFAULT " + ptr->_default;
				}
				if (doc[t].isMember("option")) {
					ptr->option = " " + doc[t]["option"].asString();
					str += ptr->option;
				}
				m[ptr->name] = ptr;
				nCount++;
				if (strall.empty())	strall = str;
				else					strall += (",\n" + str);
			}
			catch (std::exception& e) {
				Log("%-32s %s %s", __func__, t.c_str(), e.what());
			}
		}
		if (strall.length()) {
			if (nCountOfColumn)	ddl += ",\n";
			ddl += ("\n" + strall);
		}
		if (bTail)	ddl += "\n);\n";
		return nCount;
	}
	void		SetIndex(PCSTR pTable, ColumnMap& m, std::string& ddl) {
		std::string		str;
		for (auto& t : m) {
			str += "CREATE INDEX IF NOT EXISTS [" + (std::string)pTable + ".idx." + t.second->name + "] ON [" + pTable + "]([" +
				t.second->name + "]";
			if (t.second->option.length())
				str += " " + t.second->option;
			str += ");\n";
		}
		ddl += str;
	}
	void		GetColumn(DWORD dwFlags, std::function<void(int nIndex, CDbColumn *p)> pCallback) {
		int		nIndex	= 0;
		if( CDB_COLUMN_KEY & dwFlags ) {
			for (auto& t : m_column.key) {
				if( pCallback )	pCallback(nIndex++, t.second.get());
			}
		}
		if( CDB_COLUMN_INDEX & dwFlags ) {
			for (auto& t : m_column.index) {
				if (pCallback)	pCallback(nIndex++, t.second.get());
			}
		}
		if( CDB_COLUMN_DATA & dwFlags ) {
			for (auto& t : m_column.data) {
				if (pCallback)	pCallback(nIndex++, t.second.get());
			}
		}
		if (CDB_COLUMN_COUNT & dwFlags) {
			for (auto& t : m_column.count) {
				if (pCallback)	pCallback(nIndex++, t.second.get());
			}
		}
		if (CDB_COLUMN_SUM & dwFlags) {
			for (auto& t : m_column.sum) {
				if (pCallback)	pCallback(nIndex++, t.second.get());
			}
		}
	}
	void		CreateDDL(IN std::string table, IN Json::Value & req, OUT Json::Value &res) {
		//	json 정의문을 통해 table 생성 구문을 만든다. 
		//	query (insert, select, update ) 도 포함.
		try {
			Json::Value	&key	= GetJson(__func__, req, "key");
			Json::Value	&index	= GetJson(__func__, req, "index");
			Json::Value	&data	= GetJson(__func__, req, "data");
			Json::Value	&count	= GetJson(__func__, req, "count");
			Json::Value &sum	= GetJson(__func__, req, "sum");
			struct {
				std::string		ddl;
				std::string		select;
				std::string		selectall;
				std::string		count;
				std::string		where;
				std::string		insert;
				std::string		update;
				std::string		_delete;
				std::string		icolumn;
				std::string		scolumn;
				std::string		drop;
			} query;

			query.ddl	= "CREATE TABLE IF NOT EXISTS [" + table + "] (";

			int		nColumn	= 0;
			nColumn	+= SetColumn(m_column.key, query.ddl, nColumn, key, CDB_COLUMN_KEY, false);
			nColumn += SetColumn(m_column.index, query.ddl, nColumn, index, CDB_COLUMN_INDEX, false);
			nColumn	+= SetColumn(m_column.data, query.ddl, nColumn, data, CDB_COLUMN_DATA, false);
			nColumn	+= SetColumn(m_column.count, query.ddl, nColumn, count, CDB_COLUMN_COUNT, false);
			nColumn += SetColumn(m_column.sum, query.ddl, nColumn, sum, CDB_COLUMN_SUM, true);
			SetIndex(table.c_str(), m_column.index, query.ddl);

			res["ddl"]	= query.ddl;

			query.where	= " where ";
			GetColumn(CDB_COLUMN_KEY, [&](int nIndex, CDbColumn * p) {
				if (nIndex)	query.where += " and ";
				query.where += (p->name + "=?");
			});
			GetColumn(CDB_COLUMN_KEY|CDB_COLUMN_INDEX|CDB_COLUMN_DATA|CDB_COLUMN_COUNT|CDB_COLUMN_SUM, 
				[&](int nIndex, CDbColumn* p) {
				if( nIndex )	query.icolumn	+= ",";
				query.icolumn	+= p->name;
			});	
			GetColumn(CDB_COLUMN_KEY|CDB_COLUMN_INDEX|CDB_COLUMN_COUNT|CDB_COLUMN_SUM, [&](int nIndex, CDbColumn* p) {

				if (nIndex)	query.scolumn += ",";
				query.scolumn += p->name;
			});

			query.drop		= "DROP TABLE IF EXISTS [" + table + "]";
			query.select	= "select " + query.scolumn + " from " + table + query.where;
			query.selectall	= "select " + query.scolumn + " from " + table;
			query.count		= "select ";
			GetColumn(CDB_COLUMN_KEY, [&](int nIndex, CDbColumn* p) {
				query.count += ("count(" + p->name + ")");
			});
			query.count	+= " from " + table + query.where;
			query.insert	= "insert into " + table + "(" + query.icolumn + ") values(";
			GetColumn(CDB_COLUMN_KEY|CDB_COLUMN_INDEX|CDB_COLUMN_DATA|CDB_COLUMN_COUNT|CDB_COLUMN_SUM, 
				[&](int nIndex, CDbColumn* p) {
				if( nIndex )	query.insert	+= ",";
				query.insert	+= "?";
			});
			query.insert	+= ")";
			query._delete	= "delete from " + table + query.where;
			query.update	= "update " + table + " set ";
			int		nCountOfColumn	= 0;
			GetColumn(CDB_COLUMN_INDEX, [&](int nIndex, CDbColumn *p) {
				if( nIndex)	query.update	+= ",";
				query.update += (p->name + "=?");
				nCountOfColumn++;
			});
			GetColumn(CDB_COLUMN_COUNT, [&](int nIndex, CDbColumn* p) {
				if (nCountOfColumn || nIndex)	query.update += ",";
				query.update += (p->name + "=" + p->name + "+?");
				nCountOfColumn++;
			});
			GetColumn(CDB_COLUMN_SUM, [&](int nIndex, CDbColumn* p) {
				if (nCountOfColumn || nIndex)	query.update += ",";
				query.update += (p->name + "=" + p->name + "+?");
				nCountOfColumn++;
			});
			query.update	+= query.where;

			res["query"]["s"]	= query.select;
			res["query"]["i"]	= query.insert;
			res["query"]["d"]	= query._delete;
			res["query"]["u"]	= query.update;
			res["query"]["c"]	= query.count;
			res["drop"]			= query.drop;
			res["query"]["a"]	= query.selectall;
		}
		catch (std::exception& e) {
			Log("%-32s %s", __func__, e.what());
		}
	}
	bool		Initialize() {
		bool	bRet	= false;

		std::wstring	path;

		switch (m_config.storageType) {
			case StorageMemory:
				path	= L":memory:";
				break;
			default:
				path	=m_path.cdb;
				break;
		}
		if( Open(path.c_str(), __func__) ) {
			if( StorageFile == m_config.storageType )
				Log("%-32s %-20s %ws", __func__, "file db", path.c_str());
			else
				Log("%-32s %-20s", __func__, "memory db");

			//Execute(NULL, "PROGMA journal_mode=WAL");
			//Execute(NULL, "PRAGMA synchronous=NORMAL");
			sqlite3_enable_shared_cache(1);
			Execute(NULL, "PRAGMA cache_size=100000");

			if (false == m_config.bPersistent) {
				Execute([&](int nAffected, PCSTR pQuery, PCSTR pError) {
					if( nAffected )	Log("%-32s %d", "affected", nAffected);
					Log("%-32s %s", "query", pQuery);
					if (pError)	Log("%-32s %s", "error", pError);
				}, m_config.doc["drop"].asCString());
			}
			Execute([&](int nAffected, PCSTR pQuery, PCSTR pError) {
				Log("%-32s %d", "affected", nAffected);
				Log("%-32s %s", "query", pQuery);
				if( pError )	Log("%-32s %s", "error", pError);
			}, m_config.doc["ddl"].asCString());

			Json::Value	&query	= m_config.doc["query"];
			for (auto& t : query.getMemberNames() ) {
				sqlite3_stmt	*p	= Stmt(query[t].asCString());
				if (p) {
					if (!t.compare("c"))
						m_stmt.c = p;
					else if (!t.compare("i"))
						m_stmt.i = p;
					else if (!t.compare("s"))
						m_stmt.s = p;
					else if (!t.compare("u"))
						m_stmt.u = p;
					else if (!t.compare("d"))
						m_stmt.d = p;
					else if (!t.compare("a"))
						m_stmt.a = p;
					else {
						Log("%s %s\n%s", t.c_str(), query[t].asCString(), "unknown query");
						Free(p);
					}
				}
				else {
					Log("%s %s\n%s", t.c_str(), query[t].asCString(), sqlite3_errmsg(Handle()));
				}				
			}
			BeginAndCommit(false);
		}
		else {


		}
		return bRet;
	}
	void		Destroy() {
		if( IsOpened() ) {
			Commit(__func__);
			if( StorageMemory == m_config.storageType ) {
				Log("%-32s %-20s %ws", __func__, "backup", m_path.mdb.c_str());
				Backup(m_path.mdb.c_str());
			}
			Close(__func__);

			if( m_stmt.c )	Free(m_stmt.c);
			if (m_stmt.s)	Free(m_stmt.s);
			if (m_stmt.i)	Free(m_stmt.i);
			if (m_stmt.d)	Free(m_stmt.d);
			if (m_stmt.u)	Free(m_stmt.u);
			if( m_stmt.a)	Free(m_stmt.a);
			Log("%-32s %s", __func__, "db is closed.");
		}
	}
	bool		Backup(IN PCWSTR pFilePath) {
		Log("%-32s %ws", __func__, pFilePath);
		bool	bRet	= false;
		if (IsOpened() ) {

			CDB		dest;
			int		nStatus;
			if (dest.Open(pFilePath, __func__)) {
				sqlite3_exec(dest.Handle(), "PRAGMA auto_vacuum=1", NULL, NULL, NULL);

				int				rc = SQLITE_ERROR;
				sqlite3_backup* pBackup = sqlite3_backup_init(dest.Handle(), "main", Handle(), "main");
				if (pBackup)
				{
					while( SQLITE_OK == (nStatus = sqlite3_backup_step(pBackup, -1)) );
					nStatus	= sqlite3_backup_finish(pBackup);
					Log("%-32s %d", "sqlite3_backupo_finish", nStatus);

					rc = sqlite3_errcode(Handle());
					if (SQLITE_OK == rc)
					{
						bRet = true;
					}
					else
					{
						Log("%-32s %s", __func__, sqlite3_errmsg(dest.Handle()));
					}
				}
				else
				{
					Log("%-32s sqlite3_backup_init() failed. path={}",
						__func__, __ansi(pFilePath));
				}
				dest.Close(__func__);
				if (false == bRet )
				{
					//	오류가 발생된 파일은 삭제해버린다.
					Log("%-32s rc = %d", __func__, rc);
					//YAgent::DeleteFile(pFilePath);
				}
				dest.Close(__func__);
			}
		}
		else {
			Log("%-32s db is not opened.", __func__);

		}
		return bRet;
	}
	bool		Upsert(Json::Value& doc, bool bUpdateData) {
		bool	bRet	= false;

		//JsonUtil::Json2String(doc, [&](std::string& str) {
		//	m_pLog->Log("%-32s\n%s", __func__, str.c_str());
		//});

		bool	bDelete	= false;
		if (!m_config.doc["delete"].empty()) {
			Json::Value		&Delete	= m_config.doc["delete"];
			bDelete	= true;
			for (auto& t : Delete.getMemberNames() ) {
				if (Delete[t] != doc[t]) {
					bDelete	= false;
					break;
				}
			}
		}
		try {
			if( bDelete )	{
				Delete(doc);
				/*
				JsonUtil::Json2String(doc, [&](std::string& str) {
					Log("\n%s", str.c_str());
				});

				Json::Value		req;
				Select(req, [&](int nIndex, Json::Value& row) {
					HANDLE	hProcess = YAgent::GetProcessHandle(row["PID"].asInt());

					if (hProcess) {
						Log("%04d %5d [O] %s", nIndex, row["PID"].asInt(), row["ProcPath"].asCString());
						CloseHandle(hProcess);
					}
					else {
						Log("%04d %5d [X] %s", nIndex, row["PID"].asInt(), row["ProcPath"].asCString());
					}
					//JsonUtil::Json2String(row, [&](std::string& str) {
					//	Log("\n%s", str.c_str());
					//});
				});
				*/
			}
			else {

				if (IsExisting(doc)) {
					bRet	= Update(doc);
				}
				else {
					bRet	= Insert(doc);
				}
			}
		}
		catch (std::exception& e) {
			Log("%-32s %s", __func__, e.what());
		}
		BeginAndCommit(false);
		return bRet;
	}
	void		BeginAndCommit(bool bForce) {
		m_dwOP++;
		if( m_bBegin ) {
			if (bForce || 0 == m_dwOP % 300) {
				m_bBegin	= false;
				Log("COMMIT");
				Commit(__func__);
			}
		}

		if (false == m_bBegin) {
			Begin(__func__);
			m_bBegin	= true;
			Log("BEGIN");
		}
	}
	int			BindColumn(PCSTR pCause, CDbColumn *p, int nIndex, sqlite3_stmt * pStmt, 
					const Json::Value & doc, bool bLog = false) {
		try {
			switch (p->cType) {
				case CDB_COLUMN_DATA:
					//	doc안에 해당 컬럼명이 없을 수 있다. 
					//	다소 가상의 값이라고나 할까?
					if (ColumnTypeDATA == p->dType ) {
						//	없어도 된다. 아래에서 처리된다.
					}
					else {
						//	없는 경우 넘어가준다. 
						sqlite3_bind_null(pStmt, nIndex);
						return nIndex;
					}
					break;
				default:
					if (!doc.isMember(p->name)) {
						Log("%-10s %-32s cType=%d, dType=%d, not found.", 
							pCause, p->name.c_str(), p->cType, p->dType);
						sqlite3_bind_null(pStmt, nIndex);
						JsonUtil::Json2String(doc, [&](std::string& str) {
							Log("\n%s", str.c_str());
						});
						return nIndex;
					}
					break;
			}			
			switch (p->dType) {
				case ColumnTypeINT32:
					if( bLog )	Log("%2d %-32s %d", nIndex, p->name.c_str(), doc[p->name].asInt());
					sqlite3_bind_int(pStmt, nIndex, doc[p->name].asInt());
					break;
				case ColumnTypeUINT32:
					if (bLog)	Log("%02d %-32s %d", nIndex, p->name.c_str(), doc[p->name].asUInt());
					sqlite3_bind_int(pStmt, nIndex, doc[p->name].asUInt());
					break;
				case ColumnTypeINT64:
					if (bLog)	Log("%02d %-32s %I64d", nIndex, p->name.c_str(), doc[p->name].asInt64());
					sqlite3_bind_int64(pStmt, nIndex, doc[p->name].asInt64());
					break;
				case ColumnTypeUINT64:
					if (bLog)	Log("%02d %-32s %I64u", nIndex, p->name.c_str(), doc[p->name].asUInt64());
					//	JSON에는 UINT64가 있지만 
					//	sqlite3엔 UINT64가 없기에 signed로 바꿔서 넣는다.
					//	그러므로 sqlite3 -> json에 넣을 때도 형에 따라 바꿔서 넣어줘야 한다.
					sqlite3_bind_int64(pStmt, nIndex, (int64_t)doc[p->name].asUInt64());
					break;
				case ColumnTypeNVARCHAR2:
				case ColumnTypeTEXT:
					if (bLog)	Log("%02d %-32s %s", nIndex, p->name.c_str(), doc[p->name].asCString());
					sqlite3_bind_text(pStmt, nIndex, doc[p->name].asCString(), -1, SQLITE_TRANSIENT);
					break;

				case ColumnTypeDATA:
					if (bLog)	Log("%02d %-32s %s", nIndex, p->name.c_str(), "DATA");
					JsonUtil::Json2String(doc, [&](std::string& str) {
						sqlite3_bind_text(pStmt, nIndex, str.c_str(), -1, SQLITE_TRANSIENT);
					});
					break;
			}
		}
		catch (std::exception& e) {
			Log("%-10s %-32s %s", pCause, p->name.c_str(), e.what());
		} 
		return nIndex;
	}
	bool		IsExisting(Json::Value& doc) {
		int				nCount = 0;
		if (m_stmt.c) {
			int	nStatus;
			GetColumn(CDB_COLUMN_KEY, [&](int nIndex, CDbColumn* p) {
				BindColumn("IsExisting", p, nIndex+1, m_stmt.c, doc);
			});
			if (SQLITE_ROW == (nStatus = sqlite3_step(m_stmt.c))) {

				nCount = sqlite3_column_int(m_stmt.c, 0);
				//m_pLog->Log("%-32s nCount=%d", __func__, nCount);
			}
			else {
				//m_pLog->Log("%-32s nStatus=%d", __func__, nStatus);

			}
			sqlite3_reset(m_stmt.c);
		}
		else {
			Log("%-32s %s", __func__, sqlite3_errmsg(Handle()));
		}
		return nCount ? true : false;
	}
	bool		Insert(Json::Value& doc) {
		bool	bRet	= false;
		if (m_stmt.i) {
			GetColumn(CDB_COLUMN_KEY|CDB_COLUMN_INDEX|CDB_COLUMN_DATA|CDB_COLUMN_COUNT|CDB_COLUMN_SUM, 
				[&](int nIndex, CDbColumn* p) {
				BindColumn("Insert", p, nIndex + 1, m_stmt.i, doc);
			});
			if (SQLITE_DONE == sqlite3_step(m_stmt.i)) {
				//m_pLog->Log("%-32s %s", __func__, "inserted");
				bRet	= true;
			}
			else {
				JsonUtil::Json2String(doc, [&](std::string& str) {
					Log("%-32s %s\n%s", __func__, sqlite3_errmsg(Handle()), str.c_str());
				});
			}
			Reset(m_stmt.i);
		}
		else {
			Log("%-32s %s", __func__, "m_stmt.i is null.");
		}
		return bRet;
	}
	bool		Update(Json::Value& doc) {
		bool	bRet	= false;
		sqlite3_stmt	*p	= m_stmt.u;
		int		nStart	= 0;
		if (p) {			
			GetColumn(CDB_COLUMN_INDEX,
				[&](int nIndex, CDbColumn* p) {
				BindColumn("Update", p, ++nStart, m_stmt.u, doc, false);
			});
			GetColumn(CDB_COLUMN_COUNT,
				[&](int nIndex, CDbColumn* p) {
				BindColumn("Update", p, ++nStart, m_stmt.u, doc, false);
			});
			GetColumn(CDB_COLUMN_SUM,
				[&](int nIndex, CDbColumn* p) {
				BindColumn("Update", p, ++nStart, m_stmt.u, doc, false);
			});
			GetColumn(CDB_COLUMN_KEY,
				[&](int nIndex, CDbColumn* p) {
				BindColumn("Update", p, ++nStart, m_stmt.u, doc, false);
			});
			if (SQLITE_DONE == sqlite3_step(p)) {
				bRet	= true;

				//JsonUtil::Json2String(doc, [&](std::string& str) {
				//	int	nChanged	= sqlite3_changes(Handle());
				//	Log("%-32s changed:%d\n%s", "Update", nChanged, str.c_str());
				//});
			}
			else {
				Log("%-32s %s", __func__, sqlite3_errmsg(Handle()));
			}
			Reset(p);
		}
		return bRet;
	}
	bool		Delete(Json::Value& doc) {
		bool	bRet	= false;
		if (m_stmt.d) {
			int	nStatus;
			GetColumn(CDB_COLUMN_KEY, [&](int nIndex, CDbColumn* p) {
				BindColumn("Delete", p, nIndex + 1, m_stmt.d, doc);
			});
			if (SQLITE_DONE == (nStatus = sqlite3_step(m_stmt.d))) {
				
				//Log("%-32s changed:%d", __func__, sqlite3_changes(Handle()));
				bRet	= true;
			}
			else {
				Log("%-32s nStatus=%d", __func__, nStatus);
			}
			sqlite3_reset(m_stmt.d);
		}
		else {
			Log("%-32s %s", __func__, sqlite3_errmsg(Handle()));
		}
		return bRet;
	}
	int			Select(const Json::Value& req, 
				std::function<void(int nIndex, Json::Value & res)>	pCallback) {
		int			nRowCount	= 0;
		int			nKeyCount	= 0;

		GetColumn(CDB_COLUMN_KEY, [&](int nIndex, CDbColumn* p) {
			if (req.isMember(p->name)) {
				//	주어진 요청문에 KEY 항목이 존재하면 개수를 세어본다.
				nKeyCount++;
			}
		});

		sqlite3_stmt* pStmt = nKeyCount? m_stmt.s : m_stmt.a;
		if (pStmt) {
			if( nKeyCount ) {
				GetColumn(CDB_COLUMN_KEY,
					[&](int nIndex, CDbColumn* p) {
					BindColumn("Insert", p, nIndex + 1, pStmt, req);
				});
			}
			int	nColumnCount	= sqlite3_column_count(pStmt);
			Json::Value	column;
			for (auto i = 0; i < nColumnCount; i++) {
				Json::Value		c;
				const char	*pColumnName	= sqlite3_column_name(pStmt, i);
				const char	*pColumnType	= sqlite3_column_decltype(pStmt, i);

				c["name"] = pColumnName ? pColumnName : Json::Value::null;
				pColumnType = sqlite3_column_decltype(pStmt, i);
				c["type"] = pColumnType ? pColumnType : Json::Value::null;
				column.append(c);
			}
			int		nStatus;
			for (int nRow = 0; nRow < 1000; nRow++) {
				Json::Value		row;
				nStatus = sqlite3_step(pStmt);
				if (SQLITE_ROW == nStatus) {
					row.clear();
					for (unsigned i = 0; i < column.size(); i++) {
						try {
							auto& t = column[i];
							const	Json::Value& type = t["type"];
							const	Json::Value& name = t["name"];

							const	unsigned char* pValue = NULL;
							const	int				nValue = 0;
							const	int64_t			nValue64 = 0;

							int						nType = sqlite3_column_type(pStmt, i);
							if (0 == nRow) {
								//	column 정보를 넣을 때 한번에 하려 했으나 
								//	윗 부분에서 하면 모두 type == 5로 나온다.
								//	실데이터가 필요해.
								column[i]["@type"] = nType;
							}
							switch (nType) {
							case SQLITE_INTEGER:
								row[name.asString()]	= (uint64_t)sqlite3_column_int64(pStmt, i);
								break;

							case SQLITE_FLOAT:
								row[name.asString()]	= sqlite3_column_double(pStmt, i);
								break;

							case SQLITE_TEXT:
								row[name.asString()]	= (char*)sqlite3_column_text(pStmt, i);
								break;

							case SQLITE_NULL:
								row[name.asString()]	= Json::Value::null;
								break;

							case SQLITE_BLOB:
								row[name.asString()]	= "not supported value type";
								break;
							}
						}
						catch (std::exception& e) {
							Log("%-32s %s", __func__, e.what());
						}
					}
					if (pCallback) {
						pCallback(nRowCount, row);
					}
					nRowCount++;
				}
				else if (SQLITE_DONE == nStatus) {
					break;
				}
				else
					break;
			}
			//JsonUtil::Json2String(res, [&](std::string& str) {
			//	Log("%-32s %s\n%s", __func__, sqlite3_errmsg(Handle()), str.c_str());
			//});
			Reset(m_stmt.s);
		}
		else {
			Log("%-32s %s", __func__, "m_stmt.i is null.");
		}
		return nRowCount;
	}
private:	
	std::atomic<DWORD>		m_dwOP;
	std::atomic<bool>		m_bBegin;
	struct {
		std::string			db;
		std::string			table;
		Json::Value			doc;
		CDbStorageType		storageType;
		bool				bPersistent;
		DWORD				dwTransaction;
	} m_config;

	struct {
		std::wstring	root;
		std::wstring	odb;
		std::wstring	cdb;
		std::wstring	mdb;
	} m_path;

	struct {
		ColumnMap		key;
		ColumnMap		index;
		ColumnMap		data;
		ColumnMap		count;
		ColumnMap		sum;
	} m_column;
	
	struct {
		sqlite3_stmt	*i;
		sqlite3_stmt	*s;
		sqlite3_stmt	*c;
		sqlite3_stmt	*d;
		sqlite3_stmt	*u;
		sqlite3_stmt	*a;			//	select all
	} m_stmt;
};

typedef std::shared_ptr<CDbClass>			DbClassPtr;
typedef std::map<std::string, DbClassPtr>	DbClassMap;
typedef std::list<DbClassPtr>				DbClassList;
typedef std::map<std::string, DbClassList>	DbClassListMap;


#define DBCLASS_PROCESS_CRC		41800



class CDbClassFactory
{
public:
	CDbClassFactory() 
		:
		m_log(L"dbclassfactory.log")
	{

	}
	virtual	~CDbClassFactory() {

	}
	void	Initialize(IN Json::Value & doc) {
		m_log.Log("%-32s", __FUNCTION__);
		for (auto& t : doc.getMemberNames()) {

			try {
				Json::Value	&db		= doc[t];
				Json::Value	&table	= db["name"];

				m_log.Log("%-32s %s", __func__, t.c_str());
				//JsonUtil::Json2String(table, [&](std::string& str) {
				//	m_log.Log("%-32s %s", __func__, str.c_str());
				//});
				if( m_db.end() == m_db.find(t) ) {
					WCHAR	szLogName[100]	= L"";
					StringCbPrintf(szLogName, sizeof(szLogName), L"cdbclassfactory.%s.log", __utf16(t));

					DbClassPtr	ptr	= std::make_shared<CDbClass>(szLogName, t, table.asString(), db);
					m_db[t]		= ptr;					
					m_table[table.asString()].push_back(ptr);
					ptr->Initialize();
				}
			}
			catch (std::exception& e) {
				m_log.Log("%-32s %-20s %s", __func__, t.c_str(), e.what());
			}
		}
	}
	void	Destroy() {
		m_log.Log("%-32s", __FUNCTION__);

		for (auto& t : m_db) {
			t.second->Destroy();
		}
		m_db.clear();
		m_table.clear();
	}
	void	Upsert(IN Json::Value& doc, bool bUpdateData) {

		bool	bDelete	= false;
		try {
			Json::Value	&name	= doc["@name"];

			auto	t = m_table.find(name.asString());
			if (m_table.end() == t) {
				m_log.Log("%-32s name [%s] is not found.", __func__, name.asCString());
			}
			else {
				for( auto &i : t->second ) {
					i->Upsert(doc, bUpdateData);
				}
			}
		}
		catch (std::exception& e) {
			m_log.Log("%-32s @category %s", __func__, e.what());
		}
	}

	//------------------------------------------------------------------------------------------------
	

private:
	CAppLog				m_log;
	DbClassMap			m_db;
	DbClassListMap		m_table;
};
