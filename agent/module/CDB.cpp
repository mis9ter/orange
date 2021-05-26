#include "CDB.h"
#include <set>

inline	PCWSTR	GetFileName(IN PCWSTR pFilePath) {
	PCWSTR	pName	= _tcsrchr(pFilePath, L'\\');
	return pName? pName + 1 : pFilePath;
}

DWORD	CPatchDB::GetColumnListString(IN const Json::Value & src, IN const Json::Value & dest, 
			OUT std::string &str) 
{
	/*
		dest	���� ������� db�� ���̺�
		src		���� ���� db�� ���̺�
		dest �� ���̺� �÷����� ������ ���� src�� ���̺� �������� �ʴ� �÷����� ���ܽ�Ų��. 	
	*/
	if( !src.isMember("column"))	return 0;
	if( !dest.isMember("column"))	return 0;

	const Json::Value	&scol	= src["column"];
	const Json::Value	&dcol	= dest["column"];

	DWORD	dwCount	= 0;
	for( auto & t : dcol ) {
		try {
			const Json::Value	&name	= t["name"];
			if( scol.isMember(name.asString()) ) {
				if( str.length() )		str	+= ",";
				str	+=	name.asString();							
				dwCount++;
			}
		}
		catch( std::exception & e ) {
			Log("%-32s %s", __func__, e.what());
		}
	}
	return dwCount;
}
DWORD	CPatchDB::CheckSchema(
	IN	CDB					&srcDb,
	IN	const Json::Value	&srcSchema,
	IN	CDB					&destDb,
	IN	const Json::Value	&destSchema
) {
	DWORD	dwCount	= 0;

	/*
		src �������� dest �� ��Ű�� �˻�. 
		dest�� �߰��� �����ϴ� ��Ű���� ���⼭ �˻��� �� ����. 	
	*/
	for( auto & t : srcSchema.getMemberNames() ) {
		try {
			const Json::Value	&src	= srcSchema[t];
			if( destSchema.isMember(t) ) {
				const Json::Value	&dest	= destSchema[t];

				if( src.isMember("sqlcrc") && dest.isMember("sqlcrc")) {
					if( src["sqlcrc"].asLargestUInt() == dest["sqlcrc"].asLargestUInt() ) {
						//	2���� ���̺�/�ε����� �����ϴ�.
						//Log("%-32s src.%s == dest.%s", __func__, t.c_str(), t.c_str());
					}
					else {
						Log("%-32s src.%s != dest.%s", __func__, t.c_str(), t.c_str());
						if( src["type"].asString().compare("table")) {
							//	���̺��� �ƴ� ��� ������ ����
							if( destDb.Execute([&](int n, PCSTR pQuery, PCSTR pErrMsg) {
								//	���� ��Ű�� �̸� ����
								if( pErrMsg )
									Log("%d %s\n%s", n, pQuery, pErrMsg);
								else 
									Log("%d %s", n, pQuery);
							}, "drop %s [%s]", t.c_str(), t.c_str()) ) {
								if( destDb.Execute([&](int n, PCSTR pQuery, PCSTR pErrMsg) {
									//	�� ��Ű�� ����
									if( pErrMsg )
										Log("%d %s\n%s", n, pQuery, pErrMsg);
									else 
										Log("%d %s", n, pQuery);
								}, src["sql"].asCString()) ) {

								}		
							}						
						}
						else {
							//	���̺�
							//	dest table/index rename �� src�� ����
							//	
							if( destDb.Execute([&](int n, PCSTR pQuery, PCSTR pErrMsg) {
								//	���� ��Ű�� �̸� ����
								if( pErrMsg )
									Log("%d %s\n%s", n, pQuery, pErrMsg);
								else 
									Log("%d %s", n, pQuery);

								}, "drop table if exists [%s_bak]", t.c_str()) ) {
								//	*.bak�� �����ϴ� ��� drop 
							}

							std::string	columns;
							GetColumnListString(src, dest, columns);

							if( destDb.Execute([&](int n, PCSTR pQuery, PCSTR pErrMsg) {
								//	���� ��Ű�� �̸� ����
								if( pErrMsg )
									Log("%d %s\n%s", n, pQuery, pErrMsg);
								else 
									Log("%d %s", n, pQuery);

							}, "alter table [%s] rename to [%s_bak]", t.c_str(), t.c_str()) ) {

								if( destDb.Execute([&](int n, PCSTR pQuery, PCSTR pErrMsg) {
									//	���ο� ��Ű���� ����
									if( pErrMsg )
										Log("%d %s\n%s", n, pQuery, pErrMsg);
									else 
										Log("%d %s", n, pQuery);

								}, src["sql"].asCString()) ) {

									//	*.bak => *���� ������ �־���. 
									if( destDb.Execute([&](int n, PCSTR pQuery, PCSTR pErrMsg) {
										if( pErrMsg )
											Log("%d %s\n%s", n, pQuery, pErrMsg);
										else 
											Log("%d %s", n, pQuery);

									}, "insert into [%s](%s) select %s from [%s_bak]", t.c_str(), columns.c_str(), 
										columns.c_str(), t.c_str()) ) {
										if( destDb.Execute([&](int n, PCSTR pQuery, PCSTR pErrMsg) {
											if( pErrMsg )
												Log("%d %s\n%s", n, pQuery, pErrMsg);
											else 
												Log("%d %s", n, pQuery);

										}, "drop %s [%s_bak]", src["type"].asCString(), t.c_str()) ) {

										}
									}								
								}
								else {
									//	���ο� ��Ű���� ���� ����
									//	���� ���� �״�� �д�.
									destDb.Execute([&](int n, PCSTR pQuery, PCSTR pErrMsg) {
										//	���� ��Ű�� �̸� ����
										if( pErrMsg )
											Log("%d %s\n%s", n, pQuery, pErrMsg);
										else 
											Log("%d %s", n, pQuery);

									}, "alter table [%s_bak] rename to [%s]", t.c_str(), t.c_str());								
								}
							}						
						}						
					}
				}
				else {
					Log("%-32s sqlcrc is not found.", __func__);
				}
			}
			else {
				Log("%-32s dest.%s is not found.", __func__, t.c_str());
				int		nAffected	= 0;
				if( destDb.Execute([&](int nAffected, PCSTR pQuery, PCSTR pErrorMsg) {
					if( pErrorMsg )
						Log("%d %s\n%s", nAffected, pQuery, pErrorMsg);
					else 
						Log("%d %s", nAffected, pQuery);				

				}, src["sql"].asCString()) ) {
					Log("%-32s dest.%s is created.", __func__, t.c_str());

				}
			}
		}
		catch( std::exception & e) {
			Log("%-32s %s", __func__, e.what());

		}
	}
	for( auto & t : destSchema.getMemberNames() ) {
		//	dest �������� src�� ��.
		//	dest �� �߰��� �����ϴ� ��Ű���� �˾Ƴ� �� �ִ�. 
		try {
			const Json::Value	&dest	= destSchema[t];
			if( srcSchema.isMember(t) ) {
				//	src�� �����ϴ� ��Ű����� ������ �̹� ó�� �Ϸ�.
			}
			else {
				//	src�� �������� �ʰ� dest���� �����ϴ� ��Ű��
				Log("%-32s dest.%s not in src", __func__, t.c_str());
				if( dest["type"].asString().compare("table")) {
					//	���̺��� �ƴ� ��� (��:�ε���) �����Ѵ�.
					if( destDb.Execute([&](int n, PCSTR pQuery, PCSTR pErrMsg) {
						if( pErrMsg )
							Log("%d %s\n%s", n, pQuery, pErrMsg);
						else 
							Log("%d %s", n, pQuery);

					}, "drop %s [%s]", dest["type"].asCString(), t.c_str()) ) {

					}
				}
				else {
					//	���̺��� ��� 
					//	���� �� �Ǵ� ���� �������� ���� ������ ���� -> �״�� ����

				}
			}
		}
		catch( std::exception & e) {
			Log("%-32s %s", __func__, e.what());

		}
	}
	return dwCount;
}
DWORD	CPatchDB::Patch(
	IN PCWSTR	pSrcPath, 
	IN PCWSTR	pDestPath
) {
	//	db ���� ��ġ �ڵ�ȭ
	//	1. pSrcPath�� db ���ϰ� pDestPath�� db ������ ��.
	//	2. pSrcPath���� ������ pDestPath���� ���� table, index ����
	//	3. pSrcPath�� �ٸ� ���̺�, �ε��� ���� ����, �������ش�. 

	Log(__func__);

	DWORD	dwCount	= 0;
	CDB		srcDb, destDb;
	if( false == srcDb.Open(pSrcPath, __func__)	||
		false == destDb.Open(pDestPath, __func__) ) {
		//	������ CDB�� �Ҹ��ڿ��� �ڵ����� �ݾƿ�.		
		return false;
	}
	WCHAR		szSrcDdlPath[AGENT_PATH_SIZE];
	WCHAR		szDestDdlPath[AGENT_PATH_SIZE];

	StringCbPrintf(szSrcDdlPath, sizeof(szSrcDdlPath), L"%s.ddl", pSrcPath);
	StringCbPrintf(szDestDdlPath, sizeof(szDestDdlPath), L"%s.ddl", pDestPath);

	Json::Value	srcSchema, destSchema;
	/*
		Table�� ���ؼ��� ���� �����Ѵ�.
	*/
	if( GetSchemaList(srcDb, true, srcSchema) ) {
		JsonUtil::Json2File(srcSchema, szSrcDdlPath);
	}
	if( GetSchemaList(destDb, true, destSchema) ) {
		JsonUtil::Json2File(destSchema, szDestDdlPath);
	}
	dwCount	= CheckSchema(srcDb, srcSchema, destDb, destSchema);

	/*
		�� Table�� ���ؼ��� ����
	*/
	srcSchema.clear(), destSchema.clear();
	if( GetSchemaList(srcDb, false, srcSchema) ) {
		JsonUtil::Json2File(srcSchema, szSrcDdlPath);
	}
	if( GetSchemaList(destDb, false, destSchema) ) {
		JsonUtil::Json2File(destSchema, szDestDdlPath);
	}
	dwCount	= CheckSchema(srcDb, srcSchema, destDb, destSchema);
	return dwCount;
}
//	 pragma table_info(WINDOW_EVENT_LOG)
bool	CPatchDB::GetColumnList(IN CDB & db, IN PCSTR pTableName, OUT Json::Value & doc)
{
	bool	bRet = true;

	const char *	pQuery = "pragma table_info(%s)";
	char			szQuery[1000];
	int				nStatus = SQLITE_OK;

	StringCbPrintfA(szQuery, sizeof(szQuery), pQuery, pTableName);
	sqlite3_stmt	*stmt = db.Stmt(szQuery);
	DWORD			dwColumnCount = 0;
	std::string		str;

	if (stmt)
	{		
		//sqlite3_bind_text(stmt, 1, pTableName, -1, SQLITE_STATIC);
		int		nColumnCount	= sqlite3_column_count(stmt);
		//Log("%s:%d", pTableName, nColumnCount);
		for (auto j = 0 ; j < 1000 && 
			SQLITE_ROW == (nStatus = sqlite3_step(stmt)) ; j++ )
		{
	
			Json::Value		row;
			row.clear();
			
			for( auto i = 0 ; i < nColumnCount ; i++ ) {
				int				nColumnType;
				const char *	pColumnName;
				char			szValue[20];

				pColumnName	= sqlite3_column_name(stmt, i);
				nColumnType	= sqlite3_column_type(stmt, i);

				switch( nColumnType ) {
				case SQLITE_INTEGER:
					row[pColumnName]	= sqlite3_column_int64(stmt, i);
					break;
				case SQLITE_FLOAT:
					row[pColumnName]	= sqlite3_column_double(stmt, i);
					break;
				case SQLITE_TEXT:
					row[pColumnName]	= (const char *)sqlite3_column_text(stmt, i);
					break;
				case SQLITE_NULL:
					row[pColumnName]	= Json::Value::null;
					break;

				default:
					StringCbPrintfA(szValue, sizeof(szValue), "unknown(%d)", nColumnType);
					row[pColumnName]	= szValue;
					break;
				}
			}
			JsonUtil::Json2String(row, [&](std::string & t) {
				//Log(t.c_str());
			});
			if( row.isMember("name") ) {
				const Json::Value	&name	= row["name"];
				if( name.isString() )
					doc[name.asString()]	= row;
			}
		}
		db.Free(stmt);
	}
	else {
		Log("%-32s %s", __func__, sqlite3_errmsg(db.Handle()));	
	}
	return bRet;
}
bool	CPatchDB::GetSchemaList(IN CDB & db, IN bool bTable, IN OUT Json::Value & doc)
{
	bool	bRet = true;

	const char *	pQuery1 = "select * from sqlite_master where type = 'table'";
	const char *	pQuery2 = "select * from sqlite_master where type <> 'table'";
	int				nStatus = SQLITE_OK;
	sqlite3_stmt	*stmt = db.Stmt(bTable? pQuery1 : pQuery2);
	DWORD			dwTableCount = 0;
	std::string		str;

	if (stmt)
	{
		for (auto j = 0 ; j < 1000 && SQLITE_ROW == (nStatus = sqlite3_step(stmt)) ; j++ )
		{
			WCHAR				szTableName[100]	= L"";
			CHAR				szName[100]			= "";
			CHAR				szType[20]			= "";
			const char			*pColumnName;
			const char			*pValue;
			int					nColumnType;
			Json::Value			row;
			int					nColumnCount	= sqlite3_column_count(stmt);

			for( auto i = 0 ; i < nColumnCount ; i++ ) {
				pColumnName	= sqlite3_column_name(stmt, i);
				nColumnType	= sqlite3_column_type(stmt, i);

				switch( nColumnType ) {
				case SQLITE_INTEGER:
					row[pColumnName]	= sqlite3_column_int64(stmt, i);
					break;
				case SQLITE_FLOAT:
					row[pColumnName]	= sqlite3_column_double(stmt, i);
					break;
				case SQLITE_TEXT:
					pValue	= (const char *)sqlite3_column_text(stmt, i);
					if( !__mstricmp(pColumnName, "name"))
						StringCbCopyA(szName, sizeof(szName), pValue);
					else if( !__mstricmp(pColumnName, "type") )
						StringCbCopyA(szType, sizeof(szType), pValue);
					row[pColumnName]	= __utf8(pValue);
					break;
				default:
					row[pColumnName]	= "unknown";
					break;
				}
			}

			if( row.isMember("sql") && row["sql"].isString() ) {
				row["sqlcrc"]	= JsonUtil::Json2CRC(row["sql"]);
			}
			if( !__mstricmp(szType, "table")) {
				GetColumnList(db, row["name"].asCString(), row["column"]);
			}
			doc[szName]	= row;
		}
		db.Free(stmt);
	}
	return bRet;
}
