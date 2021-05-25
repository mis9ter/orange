#include "CDB.h"

inline	PCWSTR	GetFileName(IN PCWSTR pFilePath) {
	PCWSTR	pName	= _tcsrchr(pFilePath, L'\\');
	return pName? pName + 1 : pFilePath;
}

DWORD	CPatchDB::Patch(
	IN PCWSTR	pSrcPath, 
	IN PCWSTR	pDestPath
) {
	//	db 파일 패치 자동화
	//	1. pSrcPath의 db 파일과 pDestPath의 db 파일을 비교.
	//	2. pSrcPath에는 있지만 pDestPath에는 없는 table, index 생성

	Log(__func__);

	CDB		srcDb, destDb;
	if( false == srcDb.Open(pSrcPath, __func__)	||
		false == destDb.Open(pDestPath, __func__) ) {
		//	열려진 CDB는 소멸자에서 자동으로 닫아요.		
		return false;
	}
	WCHAR		szSrcDdlPath[AGENT_PATH_SIZE];
	WCHAR		szDestDdlPath[AGENT_PATH_SIZE];

	StringCbPrintf(szSrcDdlPath, sizeof(szSrcDdlPath), L"%s.ddl", pSrcPath);
	StringCbPrintf(szDestDdlPath, sizeof(szDestDdlPath), L"%s.ddl", pDestPath);
	Json::Value	srcSchema, destSchema;
	if( GetSchemaList(srcDb, srcSchema) ) {
		JsonUtil::Json2File(srcSchema, szSrcDdlPath);
	}
	if( GetSchemaList(destDb, destSchema) ) {
		JsonUtil::Json2File(destSchema, szDestDdlPath);
	}

	for( auto & t : srcSchema.getMemberNames() ) {
		try {
			const Json::Value	&src	= t;
			if( destSchema.isMember(t) ) {
				const Json::Value	&dest	= destSchema[t];
			}
			else {
			
			}
			Log("%s", srcSchema[t].asCString());
		}
		catch( std::exception & e) {
			Log("%-32s %s", __func__, e.what());
		
		}
	}
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
			doc.append(row);
		}
		db.Free(stmt);
	}
	else {
		Log("%-32s %s", __func__, sqlite3_errmsg(db.Handle()));	
	}
	return bRet;
}
bool	CPatchDB::GetSchemaList(IN CDB & db, IN OUT Json::Value & doc)
{
	bool	bRet = true;

	const char *	pQuery = "select * from sqlite_master";
	int				nStatus = SQLITE_OK;
	sqlite3_stmt	*stmt = db.Stmt(pQuery);
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

			if( !__mstricmp(szType, "table")) {
				GetColumnList(db, row["name"].asCString(), row["column"]);
			}
			doc[szName]	= row;
		}
		db.Free(stmt);
	}
	return bRet;
}
