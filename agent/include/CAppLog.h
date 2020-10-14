#pragma once
#pragma warning(disable:4819)
#include "spdlog/spdlog.h"
#include "spdlog/cfg/env.h" // for loading levels from the environment variable
#include "spdlog/sinks/basic_file_sink.h"

class CAppLog
{
public:
	enum LogLevel
	{
		debug,
		info,
		warn,
		error,
		critical,
	};

	CAppLog(IN PCSTR pFilePath = AGENT_LOG_NAME)
	{
		//MakeLogPath(pFilePath);
		m_logger = spdlog::basic_logger_mt("basic", pFilePath);
	}
	~CAppLog()
	{

	}
	bool	Log(IN const char *pFmt, ...)
	{
		if (NULL == pFmt)
		{
			return DeleteFile(m_szLogPath) ? true : false;
		}
		SYSTEMTIME	st;
		GetLocalTime(&st);
		va_list		argptr;
		char		szBuf[4096] = "";
		StringCbPrintfA(szBuf, sizeof(szBuf), "%02d/%02d %02d:%02d:%02d.%03d ",
			st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
		va_start(argptr, pFmt);
		int		nLength = lstrlenA(szBuf);
		::StringCbVPrintfA(szBuf + nLength, sizeof(szBuf) - nLength, pFmt, argptr);
		va_end(argptr);
		nLength = lstrlenA(szBuf);
		puts(szBuf);
		StringCbCopyA(szBuf + nLength, sizeof(szBuf) - nLength, "\n");
		return WriteLog(szBuf, nLength + 1) ? true : false;
	}

private:
	std::shared_ptr<spdlog::logger>	m_logger;

	HANDLE		m_hFile;
	WCHAR		m_szLogPath[AGENT_PATH_SIZE];
	void		MakeLogPath(PCWSTR pFilePath)
	{
		WCHAR		szPath[AGENT_PATH_SIZE];
		WCHAR *		pStr;

		GetModuleFileName(NULL, szPath, sizeof(szPath));
		pStr = _tcsrchr(szPath, L'\\');
		if (pStr) *pStr = NULL;
		StringCbPrintf(m_szLogPath, sizeof(m_szLogPath), L"%s\\%s", szPath, pFilePath);
		printf("%ws\n", m_szLogPath);
	}
	bool		WriteLog(IN const char * pMsg, IN size_t dwSize)
	{
		HANDLE		hFile;
		DWORD		dwBytes;

		hFile = ::CreateFile(m_szLogPath, FILE_APPEND_DATA | SYNCHRONIZE, FILE_SHARE_READ, NULL, OPEN_ALWAYS,
			FILE_ATTRIBUTE_NORMAL, NULL);
		if (INVALID_HANDLE_VALUE == hFile)
		{
			return false;
		}
		::SetFilePointer(hFile, 0, NULL, FILE_END);
		::WriteFile(hFile, pMsg, (DWORD)dwSize, &dwBytes, NULL);
		::CloseHandle(hFile);
		return true;
	}
};
