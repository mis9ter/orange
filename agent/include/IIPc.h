#ifndef __IIPC_H__
#define __IIPC_H__

#include <functional>

#define IPC_PARAMS_SIZE			64

typedef enum 
{
	IPCDone,
	IPCJson,
	IPCBInary,
	IPCText,

} IPCType;

typedef struct
{
	IPCType		type;
	DWORD		dwSize;
} IPCHeader;

interface IIPCClient;
interface IIPCServer;

typedef bool	(CALLBACK * IPCServiceCallback)(
	IN	HANDLE		hClient,
	IN	IIPCClient	*pClient,
	IN void			*pContext,
	IN	HANDLE		hShutdown		//	이 핸들이 set되면 즉시 종료해야 됩니다.
);

interface IIPCServer 
{
	virtual	bool	Start(IN LPCTSTR pName, BOOL	LowIntegrityMode = FALSE, LONG		RequestThreadCount = 1) = NULL;
	virtual void	Shutdown()					= NULL;
	virtual void	SetServiceCallback(IN IPCServiceCallback pCallback, IN LPVOID pCallbackPtr)	= NULL;
};

interface IIPCClient
{
	virtual	HANDLE	Connect(IN LPCTSTR pName, IN LPCSTR pCause)	= NULL;
	virtual void	Disconnect(HANDLE hIpc, IN LPCSTR pCause)		= NULL;

	virtual DWORD	Write(
						IN PCSTR	pCause,
						IN HANDLE	hIpc,
						IN void		*pData,
						IN DWORD	dwDataSize
					)	= NULL;
	virtual DWORD	Read(
						IN PCSTR	pCause,
						IN HANDLE	hIpc,
						OUT void	*pData,
						IN DWORD	dwDataSize
					)	= NULL;

	virtual	bool	Request(
		IN PCSTR	pCause,
		IN	HANDLE	hIpc,
		IN	PVOID	pRequest,
		IN	DWORD	dwRequestSize,
		IN	std::function<void(PVOID pResponseData, DWORD dwResponseSize)> pCallback 	
	)	= NULL;

};

interface IIPC : public IIPCServer, public IIPCClient
{
	virtual IIPCServer *	GetServer()			= NULL;
	virtual IIPCClient *	GetClient()			= NULL;
	virtual void			Free(IN void *ptr = NULL)	= NULL;
	virtual	void			Release()			= NULL;
};

#endif