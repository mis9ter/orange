#include "yfilter.h"
/*
Remarks
Highest-level system-profiling drivers can call PsSetLoadImageNotifyRoutine to set up their load-image notify routines (see PLOAD_IMAGE_NOTIFY_ROUTINE).

The maximum number of drivers that can be simultaneously registered to receive load-image notifications is eight. If the maximum number of load-image notify routines is already registered when a driver calls PsSetLoadImageNotifyRoutine to try to register an additional notify routine, PsSetLoadImageNotifyRoutine fails and returns STATUS_INSUFFICIENT_RESOURCES.

Notes

An update for Windows 8.1 increases the maximum number of drivers registered to receive load-image notifications from eight to 64. This update is installed as part of a cumulative update that is available through Windows Update starting on April 8, 2014. In addition, this cumulative update is available at https://support.microsoft.com/kb/2919355.
Users of Windows 7 with Service Pack 1 (SP1) can install a hotfix to increase the maximum number of drivers registered to receive load-image notifications from eight to 64. This hotfix is available at https://support.microsoft.com/kb/2922790.
*/
#define TAG_MODULE			'udom'

void LoadImageNotifyRoutine(
	PUNICODE_STRING		FullImageName,
	HANDLE				ProcessId,
	PIMAGE_INFO			ImageInfo
)
{
	UNREFERENCED_PARAMETER(FullImageName);
	UNREFERENCED_PARAMETER(ProcessId);
	UNREFERENCED_PARAMETER(ImageInfo);

	if( NULL == Config()	||
		0 == Config()->flag.nRun	||
		0 == Config()->flag.nModule )
		return;

	CWSTR		path(FullImageName);

	if (NULL == FullImageName || ImageInfo->SystemModeImage) {
		//	커널 드라이버가 올라올 때는 ProcessId = 0 이다.
		return;
	}

	PEPROCESS	pProcess	= NULL;

	if( NT_FAILED(PsLookupProcessByProcessId(ProcessId, &pProcess)))	return;

	HANDLE		PID			= PsGetCurrentProcessId();
	struct __ARG {
		PY_MODULE_MESSAGE	pMessage;
		PUNICODE_STRING		path;
		WORD				wDataSize;
		WORD				wStringSize;
	} arg;

	arg.pMessage		= NULL;
	arg.path			= FullImageName;	
	arg.wDataSize		= sizeof(Y_MODULE_DATA);
	arg.wStringSize		= 0;
	if( ProcessTable()->IsExisting(PID, &arg, 
		[](
			bool				bCreationSaved,	
			PPROCESS_ENTRY		pEntry,	
			PVOID				pContext) 
		{
			UNREFERENCED_PARAMETER(bCreationSaved);			
			struct __ARG	*p	= (struct __ARG *)pContext;
			p->wStringSize	= sizeof(Y_MODULE_STRING);			
			p->wStringSize	+= GetStringDataSize(p->path);
			p->pMessage		= (PY_MODULE_MESSAGE)CMemory::Allocate(PagedPool, p->wDataSize + p->wStringSize, TAG_MODULE);
			if( p->pMessage ) {
				RtlZeroMemory(p->pMessage, p->wDataSize + p->wStringSize);
				p->pMessage->PID		= (DWORD)pEntry->PID;
				p->pMessage->PUID		= pEntry->PUID;
				p->pMessage->mode		= YFilter::Message::Mode::Event;
				p->pMessage->category	= YFilter::Message::Category::Module;
				p->pMessage->wRevision	= Y_MESSAGE_REVISION;
				p->pMessage->wDataSize	= p->wDataSize;
				p->pMessage->wStringSize= p->wStringSize;

				WORD		wOffset			= (WORD)(sizeof(Y_MODULE_MESSAGE));
				CopyStringData(p->pMessage, wOffset, &p->pMessage->DevicePath, p->path);
			}
		}
	)) {
		if( arg.pMessage ) {
			arg.pMessage->Properties	= ImageInfo->Properties;
			arg.pMessage->ImageSize		= (ULONG)ImageInfo->ImageSize;
			arg.pMessage->ImageBase		= ImageInfo->ImageBase;
			if (MessageThreadPool()->Push(__FUNCTION__,
				arg.pMessage->mode, arg.pMessage->category, arg.pMessage, arg.wDataSize+arg.wStringSize,false))
			{
				//	pMsg는 SendMessage 성공 후 해제될 것이다. 
				MessageThreadPool()->Alert(YFilter::Message::Category::Module);
			}
			else {
				CMemory::Free(arg.pMessage);
			}		
		}
	}
	else {
		PCWSTR	pName	= wcsrchr(path, L'\\');
		//SE_SIGNING_LEVEL;
		__log("%s %ws",	"NO PID", pName? pName + 1 : path);
		__log("  %wZ",	FullImageName);
		//__log("  PUID                %p", PUID);
		//__log("  ImageBase           %p", ImageInfo->ImageBase);
		//__log("  ImageSize           %d", (int)ImageInfo->ImageSize);
		//__log("  ImageSignatureType  %d", ImageInfo->ImageSignatureType);
		//__log("  ImageSignatureLevel %d", ImageInfo->ImageSignatureLevel);
	}
	if( pProcess )	ObDereferenceObject(pProcess);
}
bool		StartModuleFilter()
{
	__log("%-32s flag.nModule=%d", __FUNCTION__, Config()->flag.nModule);
	NTSTATUS	status = STATUS_SUCCESS;
	if (NULL == Config())	return false;

	__try
	{
		if( false == Config()->bModuleNotifyCallback ) {
			status	= PsSetLoadImageNotifyRoutine(LoadImageNotifyRoutine);
			if (NT_SUCCESS(status)) {
				Config()->bModuleNotifyCallback = true;
			}
			else {
				Config()->bModuleNotifyCallback = false;
				__log("%-32s PsSetLoadImageNotifyRoutine() failed. status=%08x", __FUNCTION__, status);
				__leave;
			}
			
		}
	}
	__finally
	{

	}
	return Config()->bModuleNotifyCallback;
}
void		StopModuleFilter(void)
{
	__log("%s", __FUNCTION__);
	if (NULL == Config())	return;

	if (Config()->bModuleNotifyCallback) {
		PsRemoveLoadImageNotifyRoutine(LoadImageNotifyRoutine);
		Config()->bModuleNotifyCallback	= false;
	}
}