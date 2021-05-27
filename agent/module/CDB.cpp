#include "CDB.h"
#include <set>

/*
	db 파일 패치 자동화
	1. pSrcPath의 db 파일[ODB]과 pDestPath의 db 파일[DB]을 비교.
	2. pSrcPath에는 있지만 pDestPath에는 없는 table, index 생성
	3. pSrcPath와 다른 테이블, 인덱스 등을 수정, 생성해준다. 
	4. pSrcPath에 없고 pDestPath에는 있는 인덱스는 삭제한다.
*/

DWORD	CPatchDB::Patch(
	IN PCWSTR	pSrcPath, 
	IN PCWSTR	pDestPath
) {
	Log(__func__);

	DWORD	dwCount	= 0;
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
	/*
		Table에 대해서만 먼저 수행한다.
	*/
	if( GetSchemaList(srcDb, true, srcSchema) ) {
		JsonUtil::Json2File(srcSchema, szSrcDdlPath);
	}
	if( GetSchemaList(destDb, true, destSchema) ) {
		JsonUtil::Json2File(destSchema, szDestDdlPath);
	}
	dwCount	= CheckSchema(srcDb, srcSchema, destDb, destSchema);
	/*
		인덱스, 트리거에 대해서도 수행
	*/
	srcSchema.clear(), destSchema.clear();
	if( GetSchemaList(srcDb, false, srcSchema) ) {
		//JsonUtil::Json2File(srcSchema, szSrcDdlPath);
	}
	if( GetSchemaList(destDb, false, destSchema) ) {
		//	테이블에 대해서만 정의서를 로그로 남기기로해요.
		//JsonUtil::Json2File(destSchema, szDestDdlPath);
	}
	dwCount	= CheckSchema(srcDb, srcSchema, destDb, destSchema);
	return dwCount;
}
DWORD	CPatchDB::GetColumnListString(IN const Json::Value & src, IN const Json::Value & dest, 
			OUT std::string &str) 
{
	/*
		src		원본 db의 테이블 정의서
		dest	현재 사용중인 db의 테이블 정의서
		dest 의 테이블 컬럼들을 가지고 오되 src의 테이블에 존재하지 않는 컬럼들은 제외시킨다. 	
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
	IN	CDB					&srcDb,			//	원본DB sqlite wrapping class
	IN	const Json::Value	&srcSchema,		//	원본DB 스키마 정의서
	IN	CDB					&destDb,		//	사용중인 DB sqlite wrapping class
	IN	const Json::Value	&destSchema		//	사용중인 DB 스키마 정의서
) {
	DWORD	dwCount	= 0;
	for( auto & t : srcSchema.getMemberNames() ) {
		/*
			src 관점에서 dest 스키마 검사. 
			src에 있지만 dest에 없는 스키마는 검사할 수 있지만,
			src에 없고 dest에 있는 스키마는 검사할 수 없다. 
		*/
		try {
			const Json::Value	&src	= srcSchema[t];
			if( destSchema.isMember(t) ) {
				const Json::Value	&dest	= destSchema[t];

				if( src.isMember("sqlcrc") && dest.isMember("sqlcrc")) {
					//	sqlcrc 는 sqlite3에서 제공되는 컬럼이 아니고
					//	sql 컬럼의 값으로 계산한 xxhash64 crc
					if( src["sqlcrc"].asLargestUInt() == dest["sqlcrc"].asLargestUInt() ) {
						//	2개의 테이블/인덱스는 동일하다.
						//Log("%-32s src.%s == dest.%s", __func__, t.c_str(), t.c_str());
					}
					else {
						Log("%-32s src.%s != dest.%s", __func__, t.c_str(), t.c_str());
						if( src["type"].asString().compare("table")) {
							//	테이블이 아닌 경우 삭제후 생성
							if( destDb.Execute([&](int n, PCSTR pQuery, PCSTR pErrMsg) {
								//	기존 스키마 이름 변경
								if( pErrMsg )
									Log("%d %s\n%s", n, pQuery, pErrMsg);
								else 
									Log("%d %s", n, pQuery);
							}, "drop %s [%s]", t.c_str(), t.c_str()) ) {
								if( destDb.Execute([&](int n, PCSTR pQuery, PCSTR pErrMsg) {
									//	새 스키마 생성
									if( pErrMsg )
										Log("%d %s\n%s", n, pQuery, pErrMsg);
									else 
										Log("%d %s", n, pQuery);
								}, src["sql"].asCString()) ) {

								}		
							}						
						}
						else {
							//	테이블
							//	dest table/index rename 후 src로 생성
							//	
							if( destDb.Execute([&](int n, PCSTR pQuery, PCSTR pErrMsg) {
								//	기존 스키마 이름 변경
								if( pErrMsg )
									Log("%d %s\n%s", n, pQuery, pErrMsg);
								else 
									Log("%d %s", n, pQuery);

								}, "drop table if exists [%s_bak]", t.c_str()) ) {
								//	*.bak이 존재하는 경우 drop 
							}
							std::string	columns;
							GetColumnListString(src, dest, columns);

							if( destDb.Execute([&](int n, PCSTR pQuery, PCSTR pErrMsg) {
								//	기존 스키마 이름 변경
								if( pErrMsg )
									Log("%d %s\n%s", n, pQuery, pErrMsg);
								else 
									Log("%d %s", n, pQuery);

							}, "alter table [%s] rename to [%s_bak]", t.c_str(), t.c_str()) ) {

								if( destDb.Execute([&](int n, PCSTR pQuery, PCSTR pErrMsg) {
									//	새로운 스키마로 생성
									if( pErrMsg )
										Log("%d %s\n%s", n, pQuery, pErrMsg);
									else 
										Log("%d %s", n, pQuery);

								}, src["sql"].asCString()) ) {
									//	*.bak => *으로 데이터 넣어줌. 
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
									//	새로운 스키마로 생성 실패
									//	기존 것을 그대로 살려준다.
									destDb.Execute([&](int n, PCSTR pQuery, PCSTR pErrMsg) {
										//	기존 스키마 이름 변경
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
		//	dest 관점에서 src와 비교.
		//	src에 없고 dest에 추가로 존재하는 스키마에 대한 처리.
		try {
			const Json::Value	&dest	= destSchema[t];
			if( srcSchema.isMember(t) ) {
				//	src에 존재하는 스키마라면 위에서 이미 처리 완료.
			}
			else {
				//	src에 존재하지 않고 dest에만 존재하는 스키마
				Log("%-32s dest.%s not in src", __func__, t.c_str());
				if( dest["type"].asString().compare("table")) {
					//	테이블이 아닌 경우 (예:인덱스) 삭제한다.
					if( destDb.Execute([&](int n, PCSTR pQuery, PCSTR pErrMsg) {
						if( pErrMsg )
							Log("%d %s\n%s", n, pQuery, pErrMsg);
						else 
							Log("%d %s", n, pQuery);

					}, "drop %s [%s]", dest["type"].asCString(), t.c_str()) ) {

					}
				}
				else {
					//	테이블인 경우 
					//	동작 중 또는 개발 과정에서 만든 것으로 생각 -> 그대로 유지
				}
			}
		}
		catch( std::exception & e) {
			Log("%-32s %s", __func__, e.what());
		}
	}
	return dwCount;
}
bool	CPatchDB::GetColumnList(IN CDB & db, IN PCSTR pTableName, OUT Json::Value & doc)
{
	//	대상 테이블의 컬럼 정보 구하기
	bool	bRet = true;

	const char *	pQuery = "pragma table_info(%s)";
	char			szQuery[1000];
	int				nStatus = SQLITE_OK;

	//	pragma 구문을 prepare 계열로 처리할 수 없어요.
	StringCbPrintfA(szQuery, sizeof(szQuery), pQuery, pTableName);
	sqlite3_stmt	*stmt = db.Stmt(szQuery);
	DWORD			dwColumnCount = 0;
	std::string		str;

	if (stmt)
	{		
		int		nColumnCount	= sqlite3_column_count(stmt);
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
bool	CPatchDB::GetSchemaList(
	IN CDB				&db,		//	알지? 그거
	IN bool				bTable,		//	true			테이블에 대해서만 스키마 얻어내기
									//	false			인덱스/트리거 등등 테이블이 아닌 스키마 얻어내기
	IN OUT Json::Value	&doc		//	event.odb.dll 와 같은 형태로 스키마를 출력
)
{
	bool	bRet = true;

	const char *	pQuery1 = "select * from sqlite_master where type = 'table'";
	const char *	pQuery2 = "select * from sqlite_master where type <> 'table' and name not like 'sqlite_%'";
	int				nStatus = SQLITE_OK;
	sqlite3_stmt	*stmt = db.Stmt(bTable? pQuery1 : pQuery2);
	DWORD			dwTableCount = 0;
	std::string		str;

	if (stmt)
	{
		for (auto j = 0 ; j < 1000 && SQLITE_ROW == (nStatus = sqlite3_step(stmt)) ; j++ )
		{
			//	코드의 오류로 무한 반복하지 못 하도록 최대 1000개로 제한

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
