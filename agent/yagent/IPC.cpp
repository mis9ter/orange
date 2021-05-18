#include "Framework.h"

bool	CAgent::IPCCallback(
	IN	HANDLE		hClient,
	IN	IIPcClient	*pClient,
	IN	void		*pContext,
	IN	HANDLE		hShutdown
) {
	IPCHeader		header;
	CAgent			*pClass	= (CAgent *)pContext;

	DWORD			dwBytes;
	bool			bResponsed	= false;

	if( dwBytes = pClient->Read(__FUNCTION__, hClient, &header, sizeof(header))) {	
		if( header.dwSize ) {
			CBuffer	data(header.dwSize);
			if( data.Data() ) {
				if( dwBytes = pClient->Read(__FUNCTION__, hClient, data.Data(), data.Size())) {
					Json::CharReaderBuilder	rbuilder;
					Json::CharReader		*reader	= NULL;
					Json::Value				req, res;
					std::string				errors;

					try {					
						reader	= rbuilder.newCharReader();
						if( reader->parse((PCSTR)data.Data(), (PCSTR)data.Data() + data.Size(), &req, &errors) ) {					
							
							pClass->Log((PSTR)data.Data());

							if( req.isMember("header") ) {
								Json::Value	&_header	= req["header"];

								if( _header.isMember("command")) {
								
									Json::Value	&_command	= _header["command"];

									

									if( !_command.asString().compare("sqlite3.query")) {
										Json::Value	&_type	= _header["type"];
										if( !_type.asString().compare("known")) {
											pClass->SetResult(__FILE__, __func__, __LINE__, res, false, 0, "type 'known' is not supported.");
										}
										else if( !_type.asString().compare("unknown")) {
											//	자유 쿼리 모드
											Json::Value	&_query	= _header["query"];
											Json::Value &_bind	= _header["bind"];
											DWORD	dwCount	= pClass->QueryUnknonwn(_query, _bind, res);
											pClass->Log("%-32s %d rows", __func__, dwCount);
										}							
										else {
											std::string	str	= "type '" + _type.asString() + "' is not supported.";
											pClass->SetResult(__FILE__, __func__, __LINE__, res, false, 0, str.c_str());
										}
									}	
									else {
										std::string	str	= "command '" + _command.asString() + "' is not supported.";
										pClass->SetResult(__FILE__, __func__, __LINE__, res, false, 0, str.c_str());
									}	
									res["header"]	= _header;
								}
								else {
									pClass->SetResult(__FILE__, __func__, __LINE__, res, false, 0, "command is not found.");							
								}							
							}
							else {
								pClass->SetResult(__FILE__, __func__, __LINE__, res, false, 0, "header is not found.");							
							}
						}
						else {
							pClass->SetResult(__FILE__, __func__, __LINE__, res, false, 0, errors.c_str());
						}
					}
					catch( std::exception & e) {
						pClass->Log("%-32s exception:%s", __FUNCTION__, e.what());
						pClass->SetResult(__FILE__, __func__, __LINE__, res, false, 0, e.what());
					}				

					std::string					str;
					Json::StreamWriterBuilder	wbuilder;
					wbuilder["indentation"]	= "\t";
					str	= Json::writeString(wbuilder, res);

					header.dwSize	= (DWORD)(str.length() + 1);
					if( pClient->Write(__FUNCTION__, hClient, &header, sizeof(header)) ) {
						pClient->Write(__FUNCTION__, hClient, (PVOID)str.c_str(), (DWORD)(str.length() + 1));
						bResponsed	= true;
					}

					if( reader )	delete reader;
					
				}
			}		
			else {
				pClass->Log("%-32s CBuffer is null", __FUNCTION__);
			}		
		}
		else {
			pClass->Log("%-32s header.type=%d header.dwSize=%d", __FUNCTION__, header.type, header.dwSize);		
		}
		if( false == bResponsed ) {
			header.type		= IPCDone;
			header.dwSize	= 0;
			pClient->Write(__FUNCTION__, hClient, &header, sizeof(header));
		}
		/*
		pClass->Log("%-32s type=%d", __FUNCTION__, header.type);
		pClass->ProcessList(pClass, 
			[&](PVOID pContext, PCWSTR pProcName, PCWSTR pProcPath, int nCount)->bool {
			Json::Value	row;

			row["ProcPath"]	= __utf8(pProcPath);
			row["ProcName"]	= __utf8(pProcName);
			row["Count"]	= nCount;
			doc.append(row);
			pClass->Log("%-32ws %d", pProcName, nCount);
			return true;
		});
		std::string					str;
		Json::StreamWriterBuilder	builder;
		builder["indentation"]	= "";
		str	= Json::writeString(builder, doc);

		if( pClient->Write(hClient, &header, sizeof(header)) ) {
			header.dwSize	= (DWORD)(str.length() + 1);
			pClient->Write(hClient, (PVOID)str.c_str(), (DWORD)(str.length() + 1));
		}
		*/
	}
	else {
		CErrorMessage	err(GetLastError());				
		pClass->Log("%-32s error:%d %s", __FUNCTION__, (DWORD)err, (PCSTR)err);
		pClient->Disconnect(hClient, __FUNCTION__);
		return false;
	}
	return true;
}
