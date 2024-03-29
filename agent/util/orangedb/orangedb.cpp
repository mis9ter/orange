﻿// reddb.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include <iostream>
#include "sqlite3.h"

#ifdef _WIN64	//_M_X64
#pragma comment(lib, "lz4lib.x64.lib")

#else
#pragma comment(lib, "lz4lib.win32.lib")

#endif


const	char* _pname = "orange";

extern "C"	void	zipvfsInit_v2();

int		sqlite3_open_db(const char* zName, sqlite3** pDb)
{
	zipvfsInit_v2();
	return sqlite3_open_v2(zName, pDb, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, _pname);
}

#if SQLITE_SHELL_IS_UTF8
extern "C" int SQLITE_CDECL __main(int argc, char** argv);
#else
extern "C" int SQLITE_CDECL __wmain(int argc, wchar_t** wargv);
#endif

#if SQLITE_SHELL_IS_UTF8
int SQLITE_CDECL main(int argc, char** argv)
#else
int SQLITE_CDECL wmain(int argc, wchar_t** wargv)
#endif
{
#ifdef TEST
	CDB		db("event.db");
	if (db.IsOpened()) {
		PCWSTR			pQuery	= L"select ProcName, ProcPath from PROCESS_CREATE_LOG";
		sqlite3_stmt	*pStmt	= db.Stmt(pQuery);
		db.Cursor(pStmt, NULL, [&](size_t nRows, PVOID pCallbackPtr, sqlite3_stmt* pStmt) -> bool {

			PCWSTR	pProcName, pProcPath;
			pProcName	= (PCWSTR)sqlite3_column_text16(pStmt, 0);
			pProcPath	= (PCWSTR)sqlite3_column_text16(pStmt, 1);

			printf("%4d %-32ws %ws\n", (int)nRows, pProcName, pProcPath);
			return true;
		});
	}
	return 0;
#else 
	#if SQLITE_SHELL_IS_UTF8
		return __main(argc, argv);
	#else
		return __wmain(argc, wargv);
	#endif
#endif
}
