#pragma once
#include <strsafe.h>

#define LBUFSIZE	4096
#define MBUFSIZE	1024
#define SBUFSIZE	128

#define PRECOMPILED_HEADER	"pch.h"
#define YAGENT_COMMON_BEGIN	namespace YAgent {
#define YAGENT_COMMON_END	};

namespace YAgent {
	BOOL		CreateDirectory(LPCTSTR lpPath);
	BOOL		MakeDirectory(LPCTSTR lpPath);
	LPCTSTR		GetURLPath(IN LPCTSTR pFilePath, OUT LPTSTR lpPath, IN DWORD dwSize);
	bool		GetRealPath(IN LPCTSTR pLinkPath, OUT LPTSTR pRealPath, IN DWORD dwSize);
	int			CompareBeginningString(IN LPCTSTR lpStr1, IN LPCTSTR lpStr2);
	int			IsStartedSameNoCase(IN LPCTSTR lpStr1, IN LPCTSTR lpStr2);
	bool		IsDirectory(LPCTSTR lpPath);
	bool		GetModulePath(OUT LPTSTR szPath, IN DWORD dwSize);
	bool		GetInstancePath(IN HINSTANCE hInstance, OUT LPTSTR szPath, IN DWORD dwSize);
	bool		GetLinkPath(IN LPCTSTR pLinkFile,
					OUT LPTSTR pLinkPath, IN DWORD dwLinkPathSize,
					OUT WIN32_FIND_DATA* pFd);
	bool		GetLinkInfo
	(
		IN	LPCTSTR		pLinkFile,
		OUT LPTSTR		pLinkPath,
		IN	DWORD		dwLinkPathSize,
		OUT WIN32_FIND_DATA* pFd,
		OUT	LPTSTR		pArgument,
		IN	DWORD		dwArgumentSize,
		OUT	LPTSTR		pWorkingDirectory,
		IN	DWORD		dwWorkingDirectorySize,
		OUT	LPTSTR		pDescription,
		IN	DWORD		dwDescriptionSize
	);

	LPCTSTR		GetFileExt(IN LPCTSTR pFilePath);
	LPCTSTR		GetFileFullExt(IN LPCTSTR pFilePath);

	DWORD64		GetFileSize64(IN HANDLE hFile);
	DWORD64		GetFileSize64ByPath(IN LPCTSTR lpPath);
	bool		FindFile(IN LPCTSTR lpPath, OUT WIN32_FIND_DATA* fd);
	bool		IsFileExist(IN LPCTSTR lpPath);
	DWORD		GetFileSize32(IN LPCTSTR lpPath, OUT bool* bExist);
	DWORD		GetFileSize(IN LPCTSTR lpPath, OUT bool* bExist);

	LPTSTR		ReplaceString(IN LPTSTR lpSrc, IN DWORD dwSrcSize,
					IN LPCTSTR lpKey, IN LPCTSTR lpRep);
};