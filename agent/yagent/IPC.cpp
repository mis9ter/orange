#include "Framework.h"

bool	CIPCCommandCallback::Sqllite3QueryUnknown
(
	PVOID pContext, 
	const Json::Value & req, 
	const Json::Value & header, 
	Json::Value & res
) {
	const Json::Value	&query	= header["query"];
	const Json::Value	&bind	= header["bind"];
	CAgent				*pClass	= (CAgent *)pContext;	
	DWORD	dwCount		= pClass->QueryUnknonwn(query, bind, res);
	pClass->Log("%-32s %d rows", __func__, dwCount);
	return true;
}
bool	CIPCCommandCallback::RegisterListener
(
	PVOID pContext, 
	const	Json::Value &req, 
	const	Json::Value	&header,
	Json::Value & res
) {

	return true;
}
bool	CAgent::IPCRecvCallback(
	IN	HANDLE		hClient,
	IN	IIPCClient	*pClient,
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
					Json::Value				req, res;
					std::string				errors;

					try {					
						Json::CharReaderBuilder	rbuilder;
						const std::unique_ptr<Json::CharReader>		
							reader(rbuilder.newCharReader());

						if( reader->parse((PCSTR)data.Data(), (PCSTR)data.Data() + data.Size(), &req, &errors) ) {					
							pClass->Log((PSTR)data.Data());
							if( req.isMember("header") ) {
								const	Json::Value	&_header	= req["header"];
								res["header"]	= req["header"];
								if( _header.isMember("command") || _header.isMember("type") ) {								
									const	Json::Value	&_command	= _header["command"];
									const	Json::Value	&_type		= _header["type"];
									if( pClass->CallCommand(
										req, _header, _command.asString(), _type.asString(), res)) {

									}
									else {
										pClass->SetResult(__FILE__, __func__, __LINE__, res, false, 0, "command callback returns false.");
									}
								}								
								else {
									pClass->SetResult(__FILE__, __func__, __LINE__, res, false, 0, "command or header is not found.");							
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

					pClass->Log(str.c_str());

					header.dwSize	= (DWORD)(str.length() + 1);
					if( pClient->Write(__FUNCTION__, hClient, &header, sizeof(header)) ) {
						pClient->Write(__FUNCTION__, hClient, (PVOID)str.c_str(), (DWORD)(str.length() + 1));
						bResponsed	= true;
					}
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
	}
	else {
		CErrorMessage	err(GetLastError());				
		pClass->Log("%-32s error:%d %s", __FUNCTION__, (DWORD)err, (PCSTR)err);
		pClient->Disconnect(hClient, __FUNCTION__);
		return false;
	}
	return true;
}