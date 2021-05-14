#include "pch.h"


HANDLE	YAgent::Run(IN PCWSTR pFilePath, IN PCWSTR pArg) {

	WCHAR	szPath[AGENT_PATH_SIZE];

	if( NULL == pFilePath || NULL == *pFilePath )	return NULL;
	if( L'"' == *pFilePath )
		StringCbCopy(szPath, sizeof(szPath), pFilePath);
	else 
		StringCbPrintf(szPath, sizeof(szPath), L"\"%s\"", pFilePath);
	if( pArg ) {
		StringCbCat(szPath, sizeof(szPath), L" ");
		StringCbCat(szPath, sizeof(szPath), pArg);
	}
	STARTUPINFO			si	= {NULL,};
	PROCESS_INFORMATION	pi	= {NULL,};

	if( CreateProcess(NULL, szPath, NULL, NULL, FALSE,   
		CREATE_BREAKAWAY_FROM_JOB|CREATE_NEW_PROCESS_GROUP|
		CREATE_DEFAULT_ERROR_MODE|CREATE_UNICODE_ENVIRONMENT,
		NULL, NULL, &si, &pi) ) {
		return pi.hProcess;		
	}
	return NULL;	
}