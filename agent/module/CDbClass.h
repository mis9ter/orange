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

typedef struct {
	std::string		name;
	std::string		type;
	std::string		option;
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
		m_pDb(NULL),
		m_pLog(pLog)
	{
		try {
			YAgent::GetDataFolder(m_path.root);
			m_path.odb	= m_path.root + L"\\" + __utf16(m_name) + L"\\.odb";
			m_path.cdb	= m_path.root + L"\\" + __utf16(m_name) + L"\\.cdb";
			m_path.mdb	= m_path.root + L"\\" + __utf16(m_name) + L"\\.mdb";
			if (PathFileExists(m_path.cdb.c_str())) {

			}
			else {

			}

			CreateTable(name, doc);
		}
		catch( std::exception & e) {
			pLog->Log("%-32s %s", __func__, e.what());
		}	
	}
	virtual	~CDbClass() {

	}

	void		CreateTable(IN std::string table, IN Json::Value& doc) {
		std::string		ddl;

		CreateDDL(table, doc, ddl);

		m_doc	= doc;
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
	void		SetColumn(IN Json::Value& doc, IN std::string name) {
		ColumnMap	*p	= NULL;
		if( !name.compare("key"))
			p	= &m_column.key;
		else if( !name.compare("index"))
			p	= &m_column.index;
		else if( !name.compare("data"))
			p	= &m_column.data;
		else
			return;

		try {
			for( auto & t : doc[name].getMemberNames() ) {
				ColumnPtr	ptr	= std::make_shared<CDbColumn>();
				ptr->name	= t;
				if(doc[name][t].isMember("type"))
					ptr->type	= doc[name][t]["type"].asString();
				if (doc[name][t].isMember("option"))
					ptr->type = doc[name][t]["option"].asString();
				(*p)[ptr->name]	= ptr;
			}
		}
		catch (std::exception& e) {
			m_pLog->Log("%-32s %s", __func__, e.what());
		}
	}

	int			SetColumn(ColumnMap & m, std::string &ddl, int nCountOfColumn, Json::Value & doc, bool bKey, bool bTail = false) {
		int				nCount	= 0;
		std::string		str;
		std::string		str2;

		if( nCountOfColumn )	ddl	+= ",\n";
		else
			ddl	+= "\n";

		
		for (auto& t : doc.getMemberNames()) {
			ColumnPtr	ptr = std::make_shared<CDbColumn>();
			ptr->name = t;

			if (str.length())	str += ",\n";
			if (doc[t].isMember("type")) {
				ptr->type = doc[t]["type"].asString();
				str += "  [" + t + "] " + ptr->type;
				if (bKey)	str += " PRIMARY KEY ";
			}		
			if (doc[t].isMember("option")) {
				ptr->option	= " " + doc[t]["option"].asString();
				str			+= ptr->option;
			}
			m[ptr->name] = ptr;
			nCount++;
		}
		ddl	+= str;
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
	void		CreateDDL(IN std::string table, IN Json::Value & doc, OUT std::string & ddl) {
		//	json 정의문을 통해 table 생성 구문을 만든다. 

		try {
			Json::Value	&key	= GetJson(__func__, doc, "key");
			Json::Value	&index	= GetJson(__func__, doc, "index");
			Json::Value	&data	= GetJson(__func__, doc, "data");

			ddl	= "CREATE TABLE IF NOT EXISTS [" + table + "] (";

			int		nColumn	= 0;
			nColumn	+= SetColumn(m_column.key, ddl, nColumn, key, true);
			nColumn += SetColumn(m_column.index, ddl, nColumn, index, false);
			nColumn	+= SetColumn(m_column.data, ddl, nColumn, data, false, true);
			SetIndex(table.c_str(), m_column.index, ddl);
			m_pLog->Log("%-32s %s", __func__, ddl.c_str());
		}
		catch (std::exception& e) {
			m_pLog->Log("%-32s %s", __func__, e.what());
		}
	}
	bool		Initialize() {
		bool	bRet	= false;

		if( Open(L":memory:", __func__) ) {


		}
		else {


		}
		return bRet;
	}

	void		Destroy() {
		if( IsOpened() )
			Close(__func__);
	}
	bool		Backup(IN PCWSTR pFilePath) {
		bool	bRet	= false;
		if (m_pDb) {

			CDB		dest;
			if (dest.Open(pFilePath, __func__)) {
				sqlite3_exec(dest.Handle(), "PRAGMA auto_vacuum=1", NULL, NULL, NULL);

				int				rc = SQLITE_ERROR;
				sqlite3_backup* pBackup = sqlite3_backup_init(dest.Handle(), "main", Handle(), "main");
				if (pBackup)
				{
					sqlite3_backup_step(pBackup, -1);
					sqlite3_backup_finish(pBackup);
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
				if (SQLITE_OK != rc)
				{
					//	오류가 발생된 파일은 삭제해버린다.
					m_pLog->Log("%-32s rc = %d", __func__, rc);
					YAgent::DeleteFile(pFilePath);
				}
				dest.Close(__func__);
			}
		}
		return bRet;
	}
	bool		Upsert(Json::Value& req) {
		bool	bRet	= false;


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
	} m_column;
	
	sqlite3				*m_pDb;
	CAppLog				*m_pLog;

};

typedef std::shared_ptr<CDbClass>			DbClassPtr;
typedef std::map<std::string, DbClassPtr>	DbClassMap;

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
				}
			}
			catch (std::exception& e) {
				m_log.Log("%-32s %s", __func__, e.what());
			}
			

		}
	}

	void	Destroy() {


	}
private:
	CAppLog		m_log;
	DbClassMap	m_table;

};
