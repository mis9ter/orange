#include "pch.h"
#include <Shlwapi.h>
#include <ShlObj.h>

PCWSTR		YAgent::GetDataFolder(IN PCWSTR pName, OUT PWSTR pValue, IN DWORD dwSize)
{

	if( SUCCEEDED(SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA, NULL, 0, pValue))) {
		PathAppend(pValue, pName);
		return pValue;	
	}
	return NULL;
}
bool	YAgent::GetAppTempPath(OUT LPTSTR szPath, IN DWORD dwSize)
{
	::GetTempPath(dwSize/sizeof(TCHAR), szPath);
	if( PathIsDirectory(szPath) )
	{
		return true;
	}
	if( ::SHGetSpecialFolderPath(NULL, szPath, CSIDL_APPDATA, TRUE) )
		return true;
	return false;
}