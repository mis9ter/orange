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


	CDbTableManager
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

class CDbClass
	:
	public	CDB
{
public:
	CDbClass
	(
		Json::Value		& doc		//	DB 정의서
	) 
	{
		try {
			YAgent::GetDataFolder(m_path.root);
			Json::Value	&name	= doc["name"];
			m_name		= __utf16(name.asCString());
			m_path.odb	= m_path.root + L"\\" + m_name + L"\\.odb";
			m_path.cdb	= m_path.root + L"\\" + m_name + L"\\.cdb";
			m_path.mdb	= m_path.root + L"\\" + m_name + L"\\.mdb";
			if (PathFileExists(m_path.cdb.c_str())) {

			}
			else {

			}
		}
		catch( std::exception & e) {
			Log("%-32s %s", __func__, e.what());
		}	
	}
	virtual	~CDbClass() {

	}

	void		CreateDDL(IN Json::Value& doc, OUT std::string &str) {
		//	json 정의문을 통해 table 생성 구문을 만든다. 

		try {
			Json::Value	&key	= doc["key"];
			Json::Value	&index	= doc["index"];
			Json::Value	&data	= doc["data"];


			str	= "CREATE TABLE [";
			str	+= doc["name"].asCString();
			str	+= "] {";



		}
		catch (std::exception& e) {
			Log("%-32s %s", __func__, e.what());
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
						Log("%-32s %s", __func__, sqlite3_errmsg(dest.Handle()));
					}
				}
				else
				{
					m_log.Log("%-32s sqlite3_backup_init() failed. path={}", 
						__func__, __ansi(pFilePath));
				}
				dest.Close(__func__);
				if (SQLITE_OK != rc)
				{
					//	오류가 발생된 파일은 삭제해버린다.
					Log("%-32s rc = %d", __func__, rc);
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
	CAppLog				m_log;
	std::wstring		m_name;
	struct {
		std::wstring	root;
		std::wstring	odb;
		std::wstring	cdb;
		std::wstring	mdb;

	} m_path;
	
	sqlite3				*m_pDb;

};
