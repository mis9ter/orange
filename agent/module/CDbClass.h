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
#include "json/json.h"
#include "CAppLog.h"
#include "CDB.h"

#define	CDBTABLE_LOG_NAME	L"cdbtable"

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
	public	CDB
{
public:
	CDbClass
	(
		std::string		& name,
		Json::Value		& doc,		//	DB 정의서
		CAppLog			* pLog
	) 
		:
		m_name(name),
		m_pLog(pLog)
	{
		try {
			ZeroMemory(&m_stmt, sizeof(m_stmt));

			YAgent::GetDataFolder(m_path.root);
			m_path.odb	= m_path.root + L"\\" + __utf16(m_name) + L".odb";
			m_path.cdb	= m_path.root + L"\\" + __utf16(m_name) + L".cdb";
			m_path.mdb	= m_path.root + L"\\" + __utf16(m_name) + L".mdb";

			CreateDDL(name, doc, m_doc);




			if (PathFileExists(m_path.cdb.c_str())) {

			}
			else {

			}
		}
		catch( std::exception & e) {
			pLog->Log("%-32s %s", __func__, e.what());
		}	
	}
	virtual	~CDbClass() {

	}

	Json::Value& GetJson(IN PCSTR pCause, IN Json::Value& doc, IN std::string name) {
		try {
			return doc[name];
		}
		catch (std::exception& e) {
			m_pLog->Log("%-32s %s:%s", pCause, name.c_str(), e.what());
			throw e;
		}
	}
	int			SetColumn(ColumnMap & m, std::string &ddl, 
					int nCountOfColumn, Json::Value & doc,DWORD dwType, bool bTail = false) {
		int				nCount	= 0;
		std::string		str, strall;
	
		for (auto& t : doc.getMemberNames()) {
			try {
				ColumnPtr	ptr = std::make_shared<CDbColumn>();
				ptr->name	= t;
				ptr->cType	= dwType;
				str.clear();

				if (doc[t].isMember("type")) {
					ptr->type	= doc[t]["type"].asString();
					if (!ptr->type.compare("INTEGER")) {
						int		nSize = doc[t]["size"]? doc[t]["size"].asInt() : 32;
						bool	bSigned = doc[t].isMember("signed") ? doc[t]["signed"].asBool() : true;

						if (64 == nSize) {
							ptr->dType	= bSigned? ColumnTypeINT64 : ColumnTypeUINT64;
						}
						else
							ptr->dType	= bSigned? ColumnTypeINT32 : ColumnTypeUINT32;
					}
					else if (!ptr->type.compare("NVARCHAR2")) {
						ptr->dType	= ColumnTypeNVARCHAR2;
					}
					else if (!ptr->type.compare("DATA")) {
						ptr->dType = ColumnTypeDATA;
						//	정의문에는 DATA로 적혀있으나, sqlite에선 TEXT를 사용한다.
						ptr->type	= "TEXT";
					}
					else if (!ptr->type.compare("TEXT")) {
						ptr->dType	= ColumnTypeTEXT;
					}
					else {
						m_pLog->Log("%-32s %s %s", __func__, "unknown type", ptr->type.c_str());
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
				if( strall.empty() )	strall	= str;
				else					strall	+= (",\n" + str);
			}
			catch (std::exception& e) {
				m_pLog->Log("%-32s %s %s", __func__, t.c_str(), e.what());
			}
		}
		if( strall.length() ) {
			if( nCountOfColumn )	ddl	+= ",\n";
			ddl	+= ("\n" + strall);
		}
		if( bTail )	ddl	+= "\n);\n";
		return nCount;
	}
	void		SetIndex(PCSTR pTable, ColumnMap &m,  std::string& ddl) {
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
			Json::Value &sum = GetJson(__func__, req, "sum");
			struct {
				std::string		ddl;
				std::string		select;
				std::string		count;
				std::string		where;
				std::string		insert;
				std::string		update;
				std::string		_delete;
				std::string		icolumn;
				std::string		scolumn;
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
			GetColumn(CDB_COLUMN_KEY|CDB_COLUMN_INDEX|CDB_COLUMN_DATA, [&](int nIndex, CDbColumn* p) {

				if( nIndex )	query.icolumn	+= ",";
				query.icolumn	+= p->name;
			});	
			GetColumn(CDB_COLUMN_KEY|CDB_COLUMN_INDEX|CDB_COLUMN_COUNT|CDB_COLUMN_SUM, [&](int nIndex, CDbColumn* p) {

				if (nIndex)	query.scolumn += ",";
				query.scolumn += p->name;
			});

			query.select	= "select " + query.scolumn + " from " + table + query.where;
			query.count		= "select ";
			GetColumn(CDB_COLUMN_KEY, [&](int nIndex, CDbColumn* p) {
				query.count += ("count(" + p->name + ")");
			});
			query.count	+= " from " + table + query.where;
			query.insert	= "insert into " + table + "(" + query.icolumn + ") values(";
			GetColumn(CDB_COLUMN_KEY|CDB_COLUMN_INDEX|CDB_COLUMN_DATA, [&](int nIndex, CDbColumn* p) {
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

			res["query"]["s"] = query.select;
			res["query"]["i"] = query.insert;
			res["query"]["d"] = query._delete;
			res["query"]["u"] = query.update;
			res["query"]["c"] = query.count;

			JsonUtil::Json2String(m_doc, [&](std::string& str) {
				m_pLog->Log("%-32s %s", __func__, str.c_str());

			});
		}
		catch (std::exception& e) {
			m_pLog->Log("%-32s %s", __func__, e.what());
		}
	}
	void		CreateSTMT() {

	}
	bool		Initialize() {
		bool	bRet	= false;
		if( Open(L":memory:", __func__) ) {

			m_pLog->Log("%-32s %s", __func__, "memory db created.");
			Execute([&](int nAffected, PCSTR pQuery, PCSTR pError) {
				m_pLog->Log("%-32s %d", "affected", nAffected);
				m_pLog->Log("%-32s %s", "query", pQuery);
				if( pError )	m_pLog->Log("%-32s %s", "error", pError);
			}, m_doc["ddl"].asCString());

			Json::Value	&query	= m_doc["query"];
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
					else {
						m_pLog->Log("%s %s\n%s", t.c_str(), query[t].asCString(), "unknown query");
						Free(p);
					}
				}
				else {
					m_pLog->Log("%s %s\n%s", t.c_str(), query[t].asCString(), sqlite3_errmsg(Handle()));
				}				
			}
		}
		else {


		}
		return bRet;
	}
	void		Destroy() {
		if( IsOpened() ) {
			Backup(m_path.mdb.c_str());
			Close(__func__);

			if( m_stmt.c )	Free(m_stmt.c);
			if (m_stmt.s)	Free(m_stmt.s);
			if (m_stmt.i)	Free(m_stmt.i);
			if (m_stmt.d)	Free(m_stmt.d);
			if (m_stmt.u)	Free(m_stmt.u);

			m_pLog->Log("%-32s %s", __func__, "db is closed.");
		}
	}
	bool		Backup(IN PCWSTR pFilePath) {
		m_pLog->Log("%-32s %ws", __func__, pFilePath);
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
					m_pLog->Log("%-32s %d", "sqlite3_backupo_finish", nStatus);

					rc = sqlite3_errcode(Handle());
					if (SQLITE_OK == rc)
					{
						bRet = true;
					}
					else
					{
						m_pLog->Log("%-32s %s", __func__, sqlite3_errmsg(dest.Handle()));
					}
				}
				else
				{
					m_pLog->Log("%-32s sqlite3_backup_init() failed. path={}",
						__func__, __ansi(pFilePath));
				}
				dest.Close(__func__);
				if (false == bRet )
				{
					//	오류가 발생된 파일은 삭제해버린다.
					m_pLog->Log("%-32s rc = %d", __func__, rc);
					//YAgent::DeleteFile(pFilePath);
				}
				dest.Close(__func__);
			}
		}
		else {
			m_pLog->Log("%-32s db is not opened.", __func__);

		}
		return bRet;
	}
	bool		Upsert(Json::Value& doc, bool bUpdateData) {
		bool	bRet	= false;

		//JsonUtil::Json2String(doc, [&](std::string& str) {
		//	m_pLog->Log("%-32s\n%s", __func__, str.c_str());
		//});
		try {
			if (IsExisting(doc)) {
				bRet	= Update(doc);
			}
			else {
				bRet	= Insert(doc);
			}
		}
		catch (std::exception& e) {
			m_pLog->Log("%-32s %s", __func__, e.what());
		}
		return bRet;
	}
	int			BindColumn(PCSTR pCause, CDbColumn *p, int nIndex, sqlite3_stmt * pStmt, 
					Json::Value & doc, bool bLog = false) {
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
						m_pLog->Log("%-10s %-32s cType=%d, dType=%d, not found.", 
							pCause, p->name.c_str(), p->cType, p->dType);
						sqlite3_bind_null(pStmt, nIndex);
						return nIndex;
					}
					break;
			}			
			switch (p->dType) {
				case ColumnTypeINT32:
					if( bLog )	m_pLog->Log("%2d %-32s %d", nIndex, p->name.c_str(), doc[p->name].asInt());
					sqlite3_bind_int(pStmt, nIndex, doc[p->name].asInt());
					break;
				case ColumnTypeUINT32:
					if (bLog)	m_pLog->Log("%02d %-32s %d", nIndex, p->name.c_str(), doc[p->name].asUInt());
					sqlite3_bind_int(pStmt, nIndex, doc[p->name].asUInt());
					break;
				case ColumnTypeINT64:
					if (bLog)	m_pLog->Log("%02d %-32s %I64d", nIndex, p->name.c_str(), doc[p->name].asInt64());
					sqlite3_bind_int64(pStmt, nIndex, doc[p->name].asInt64());
					break;
				case ColumnTypeUINT64:
					if (bLog)	m_pLog->Log("%02d %-32s %I64u", nIndex, p->name.c_str(), doc[p->name].asUInt64());
					//	JSON에는 UINT64가 있지만 
					//	sqlite3엔 UINT64가 없기에 signed로 바꿔서 넣는다.
					//	그러므로 sqlite3 -> json에 넣을 때도 형에 따라 바꿔서 넣어줘야 한다.
					sqlite3_bind_int64(pStmt, nIndex, (int64_t)doc[p->name].asUInt64());
					break;
				case ColumnTypeNVARCHAR2:
				case ColumnTypeTEXT:
					if (bLog)	m_pLog->Log("%02d %-32s %s", nIndex, p->name.c_str(), doc[p->name].asCString());
					sqlite3_bind_text(pStmt, nIndex, doc[p->name].asCString(), -1, SQLITE_TRANSIENT);
					break;

				case ColumnTypeDATA:
					if (bLog)	m_pLog->Log("%02d %-32s %s", nIndex, p->name.c_str(), "DATA");
					JsonUtil::Json2String(doc, [&](std::string& str) {
						sqlite3_bind_text(pStmt, nIndex, str.c_str(), -1, SQLITE_TRANSIENT);
					});
					break;
			}
		}
		catch (std::exception& e) {
			m_pLog->Log("%-10s %-32s %s", pCause, p->name.c_str(), e.what());
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
			m_pLog->Log("%-32s %s", __func__, sqlite3_errmsg(Handle()));
		}
		return nCount ? true : false;
	}
	bool		Insert(Json::Value& doc) {
		bool	bRet	= false;
		if (m_stmt.i) {
			GetColumn(CDB_COLUMN_KEY|CDB_COLUMN_INDEX|CDB_COLUMN_DATA, 
				[&](int nIndex, CDbColumn* p) {
				BindColumn("Insert", p, nIndex + 1, m_stmt.i, doc);
			});
			if (SQLITE_DONE == sqlite3_step(m_stmt.i)) {
				//m_pLog->Log("%-32s %s", __func__, "inserted");
				bRet	= true;
			}
			else {
				JsonUtil::Json2String(doc, [&](std::string& str) {
					m_pLog->Log("%-32s %s\n%s", __func__, sqlite3_errmsg(Handle()), str.c_str());
				});
			}
			Reset(m_stmt.i);
		}
		else {
			m_pLog->Log("%-32s %s", __func__, "m_stmt.i is null.");
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
				BindColumn("Update", p, ++nStart, m_stmt.u, doc, true);
			});
			GetColumn(CDB_COLUMN_COUNT,
				[&](int nIndex, CDbColumn* p) {
				BindColumn("Update", p, ++nStart, m_stmt.u, doc, true);
			});
			GetColumn(CDB_COLUMN_SUM,
				[&](int nIndex, CDbColumn* p) {
				BindColumn("Update", p, ++nStart, m_stmt.u, doc, true);
			});
			GetColumn(CDB_COLUMN_KEY,
				[&](int nIndex, CDbColumn* p) {
				BindColumn("Update", p, ++nStart, m_stmt.u, doc, true);
			});
			if (SQLITE_DONE == sqlite3_step(p)) {
				bRet	= true;

				JsonUtil::Json2String(doc, [&](std::string& str) {
					int	nChanged	= sqlite3_changes(Handle());
					m_pLog->Log("%-32s changed:%d\n%s", "Update", nChanged, str.c_str());
				});
			}
			else {
				m_pLog->Log("%-32s %s", __func__, sqlite3_errmsg(Handle()));
			}
			Reset(p);
		}
		return bRet;
	}
	bool		Delete(Json::Value& req) {
		bool	bRet	= false;

		return bRet;
	}
	int			Select(const Json::Value& req, Json::Value & res) {
		int		nCount	= 0;

		return nCount;
	}
private:	
	std::string			m_name;
	Json::Value			m_doc;
	
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
	} m_stmt;
	CAppLog				*m_pLog;

};

typedef std::shared_ptr<CDbClass>			DbClassPtr;
typedef std::map<std::string, DbClassPtr>	DbClassMap;


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
				Json::Value& table = doc[t];

				m_log.Log("%-32s %s", __func__, t.c_str());
				JsonUtil::Json2String(table, [&](std::string& str) {
					m_log.Log("%-32s %s", __func__, str.c_str());
				});

				if( m_table.end() == m_table.find(t) ) {
					DbClassPtr	ptr	= std::make_shared<CDbClass>(t, table, &m_log);
					m_table[t]		= ptr;
					ptr->Initialize();
				}
			}
			catch (std::exception& e) {
				m_log.Log("%-32s %s", __func__, e.what());
			}
			

		}
	}
	void	Destroy() {
		m_log.Log("%-32s", __FUNCTION__);

		for (auto& t : m_table) {
			t.second->Destroy();
		}
	}
	void	Upsert(IN Json::Value& doc, bool bUpdateData) {

		try {
			switch (doc["@crc"].asInt()) {
				case 41800:	//	process
					{
						auto	t	= m_table.find("process");
						if (m_table.end() == t) {

						}
						else {
							t->second->Upsert(doc, bUpdateData);
						}
					}
					break;
			}
		}
		catch (std::exception& e) {
			m_log.Log("%-32s\n%s", __func__, e.what());
		}

	}
private:
	CAppLog		m_log;
	DbClassMap	m_table;

};
