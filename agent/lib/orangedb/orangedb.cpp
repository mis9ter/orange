// reddb.cpp : 정적 라이브러리를 위한 함수를 정의합니다.
//
#include "pch.h"
#include "framework.h"
#include "cdb.h"

const	char* _pname = "orange";

extern "C"	void	zipvfsInit_v2();

int		sqlite3_open_db(const char* zName, sqlite3** pDb)
{
	zipvfsInit_v2();
	return sqlite3_open_v2(zName, pDb, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, _pname);
}