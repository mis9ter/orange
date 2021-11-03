#pragma once

/*
	table.json�� ���ǵ� ������ �ڵ����� sqlite table�� ����/�������ش�.
	�����ڴ� sqlite table�� ������� class�� ó���Ѵ�. 

	1. �⺻������ row���� memory���� ó���ϰ� �ʹ�. 
	2. �ʿ信 ���� row���� file�� ����߸��� �ʹ�. 


	�̰� �� sqlite�� ����� ������ ������?
	�׳� map���� �����ع���??

	���α׷� ����� ���Ϸ� save�ϴ� ��츦 �����ϸ� sqlite�� ���� �͵� ����. 




	CDbTable 
	1���� db�� ���� Ŭ����


	CDbTableManager
	1. CDbTable.json ����/�б�
	2. 1�� ���� �ʿ��Ѹ�ŭ CDbTable ����
	3. �ܺη� CDbTable ����

	��������� �ܺ� �Ե��� sqlite�� ���ؼ� �𸣰� �ȴ�. 

	�������� db�� �����ؾ� �ϸ� ��� ����???




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
		Json::Value		& doc		//	DB ���Ǽ�
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
		//	json ���ǹ��� ���� table ���� ������ �����. 

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
					//	������ �߻��� ������ �����ع�����.
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
