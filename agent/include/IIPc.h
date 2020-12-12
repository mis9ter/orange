#ifndef __IIPC_H__
#define __IIPC_H__

#define IPC_PARAMS_SIZE			64

typedef enum 
{
	IPCClose,
	IPCRequestJson,
	IPCRequestString,
	IPCResponsePacketData,
	IPCResponseData,
	IPCResponseString,
	IPCError,
	IPCResponseDone,
	IPCDummy,
} IPCType;

typedef struct
{
	IPCType		type;
	DWORD		dwSize;
} IPCHeader;

interface IIPcClient;
interface IIPcServer;

typedef bool	(CALLBACK * IPCServiceCallback)(
	IN	HANDLE		hClient,
	IN	IIPcClient	*pClient,
	IN void			*pContext,
	IN	HANDLE		hShutdown		//	이 핸들이 set되면 즉시 종료해야 됩니다.
);

interface IIPcServer 
{
	virtual	bool	Start(IN LPCTSTR pName, BOOL	LowIntegrityMode = FALSE, LONG		RequestThreadCount = 1) = NULL;
	virtual void	Shutdown()					= NULL;
	virtual void	SetServiceCallback(IN IPCServiceCallback pCallback, IN LPVOID pCallbackPtr)	= NULL;
};

interface IIPcClient
{
	virtual	HANDLE	Connect(IN LPCTSTR pName, IN LPCSTR pCause)	= NULL;
	virtual void	Disconnect(HANDLE hIpc, IN LPCSTR pCause)		= NULL;

	virtual bool	Write(
						IN HANDLE	hIpc,
						IN void		*pData,
						IN DWORD	dwDataSize
					)	= NULL;
	virtual bool	Read(
						IN HANDLE	hIpc,
						OUT void	*pData,
						IN DWORD	dwDataSize
					)	= NULL;

};

interface IIPc : public IIPcServer, public IIPcClient
{
	virtual IIPcServer *	GetServer()			= NULL;
	virtual IIPcClient *	GetClient()			= NULL;
	virtual void			Free(IN void *ptr = NULL)	= NULL;
	virtual	void			Release()			= NULL;
};

#endif