#pragma once

#include <functional>
#include "json\json.h"
#include "xxhash.h"
namespace JsonUtil {

	inline	void	Json2String(IN Json::Value & doc, 
		IN std::function<void (std::string &)> pCallback) {
		std::string					str;
		Json::StreamWriterBuilder	wbuilder;
		wbuilder["indentation"]	= "\t";
		str	= Json::writeString(wbuilder, doc);
		if( pCallback )		pCallback(str);
	}
	inline	bool	File2Json(PCWSTR pFilePath, IN Json::Value& doc) {
		DWORD	dwSize	= 0;
		char	*pContent	= (char *)YAgent::GetFileContent(pFilePath, &dwSize);
		bool	bRet = false;
		if (pContent) {
			Json::CharReaderBuilder	builder;
			const std::unique_ptr<Json::CharReader>		reader(builder.newCharReader());
			std::string	err;
			if (reader->parse(pContent, pContent + dwSize, &doc, &err)) {
				bRet = true;
			}
			YAgent::FreeFileContent(pContent);
		}
		return bRet;
	}
	inline	DWORD	Json2File(IN Json::Value & doc, PCWSTR pFilePath) {
		DWORD	dwSize	= 0;
		Json2String(doc, [&](std::string & str) {
			dwSize	= (DWORD)str.length();
			YAgent::SetFileContent(pFilePath, (PVOID)str.c_str(), dwSize);		
		});		
		return dwSize;
	}
	inline	XXH64_hash_t	Json2CRC(IN Json::Value & doc) {
		//	https://github.com/Cyan4973/xxHash
		XXH64_hash_t	nHash	= 0;
		Json2String(doc, [&](std::string & str) {
			nHash	= XXH64(str.c_str(), str.length(), 0xAB);
		});
		return nHash;	
	}
	inline	LPCSTR	GetString(
		IN const Json::Value & doc, 
		IN LPCSTR pName, 
		IN LPCSTR pDefault = "",
		IN std::function<void (const Json::Value &, PCSTR pName, PCSTR pErrMsg)> pExceptionCallback = NULL
	)
	{
		const char *	pValue = NULL;
		static char		szNull[] = "";
		if (NULL == pName || NULL == *pName)	return szNull;
		try
		{
			if (!doc.isMember(pName))
				return szNull;
			if (doc.isArray() || false == doc[pName].isString())
			{
				return pDefault ? pDefault : szNull;
			}
			pValue = doc[pName].asCString();
		}
		catch (std::exception & e)
		{
			if( pExceptionCallback ) 
				pExceptionCallback(doc, pName, e.what());
			return pDefault ? pDefault : szNull;
		}
		return pValue;
	}
	inline	bool	GetBool(IN LPCSTR pStr)
	{
		if (NULL == pStr)											return false;
		if (!_mbscmp((const unsigned char *)pStr, (const unsigned char *)"on"))			return true;
		else if (!_mbsicmp((const unsigned char *)pStr, (const unsigned char *)"true"))	return true;
		else if (atoi(pStr))				return true;
		return false;
	}
	inline	bool	GetBool(IN const Json::Value & doc, IN LPCSTR pName, IN bool bDefault = false,
		IN std::function<void (const Json::Value &, PCSTR pName, PCSTR pErrMsg)> pExceptionCallback = NULL)
	{
		bool	bRet	= bDefault;
		try
		{
			if (doc.empty() || doc.isArray())	return bRet;

			if (doc.isMember(pName))
			{
				if (doc[pName].isBool())
					bRet	= doc[pName].asBool();
				else if (doc[pName].isInt())
				{
					if( doc[pName].asInt() )
						bRet	= true;
					else 
						bRet	= false;
				}
				else if (doc[pName].isString())
				{
					if (!doc[pName].empty() )
					{
						bRet	= GetBool(doc[pName].asCString());
					}
				}
			}
		}
		catch (std::exception & e)
		{
			if( pExceptionCallback ) 
				pExceptionCallback(doc, pName, e.what());
		}
		return bRet;
	}
	inline	bool	GetBool(IN LPCTSTR pStr)
	{
		if (NULL == pStr)	return false;
		if (!_tcsicmp(pStr, _T("on")))			return true;
		else if (!_tcsicmp(pStr, _T("true")))	return true;
		else if (_ttoi(pStr))					return true;
		return false;
	}
	inline	double	GetDouble(IN const Json::Value & doc, IN LPCSTR pName, IN double dDefault = 0.0,
		IN std::function<void (const Json::Value &, PCSTR pName, PCSTR pErrMsg)> pExceptionCallback = NULL)
	{
		try {
			if (doc.isArray())	{
				return dDefault;
			}
			if (doc.isMember(pName))
			{
				if (doc[pName].isInt64()) {
					return (double)doc[pName].asInt64();
				}
				else if (doc[pName].isInt()) {
					return (double)doc[pName].asInt();
				}
				else if (doc[pName].isString())
				{
					if (doc[pName].empty()) {
						return dDefault;
					}
					else
					{
						char *	ptr = NULL;
						double	val = strtod(doc[pName].asCString(), &ptr);
						if (0 == val && ptr == doc[pName].asCString()) {
							return dDefault;
						}
						return val;
					}
				}
			}		
		}
		catch( std::exception & e) {
			if( pExceptionCallback ) 
				pExceptionCallback(doc, pName, e.what());
		}
		return dDefault;
	}
	inline	int64_t	GetInt(IN const Json::Value & doc, IN LPCSTR pName, IN int64_t nDefault = 0,
		IN std::function<void (const Json::Value &, PCSTR pName, PCSTR pErrMsg)> pExceptionCallback = NULL)
	{
		try
		{
			if (doc.isArray())	return nDefault;

			if (doc.isMember(pName))
			{
				if (doc[pName].isInt64())
					return doc[pName].asInt64();
				else if (doc[pName].isInt())
					return doc[pName].asInt();
				else if (doc[pName].isString())
				{
					if (doc[pName].empty())
						return nDefault;
					else
						return _atoi64(doc[pName].asCString());
				}
			}
		}
		catch (std::exception & e)
		{
			if( pExceptionCallback ) 
				pExceptionCallback(doc, pName, e.what());
		}
		return nDefault;
	}
};


