#include "yfilter.h"
/*
Remarks
Highest-level system-profiling drivers can call PsSetLoadImageNotifyRoutine to set up their load-image notify routines (see PLOAD_IMAGE_NOTIFY_ROUTINE).

The maximum number of drivers that can be simultaneously registered to receive load-image notifications is eight. If the maximum number of load-image notify routines is already registered when a driver calls PsSetLoadImageNotifyRoutine to try to register an additional notify routine, PsSetLoadImageNotifyRoutine fails and returns STATUS_INSUFFICIENT_RESOURCES.

Notes

An update for Windows 8.1 increases the maximum number of drivers registered to receive load-image notifications from eight to 64. This update is installed as part of a cumulative update that is available through Windows Update starting on April 8, 2014. In addition, this cumulative update is available at https://support.microsoft.com/kb/2919355.
Users of Windows 7 with Service Pack 1 (SP1) can install a hotfix to increase the maximum number of drivers registered to receive load-image notifications from eight to 64. This hotfix is available at https://support.microsoft.com/kb/2922790.
*/
NTSTATUS	AddEarlyProcess(HANDLE hPID);

void LoadImageNotifyRoutine(
	PUNICODE_STRING		FullImageName,
	HANDLE				ProcessId,
	PIMAGE_INFO			ImageInfo
)
{
	UNREFERENCED_PARAMETER(FullImageName);
	UNREFERENCED_PARAMETER(ProcessId);
	UNREFERENCED_PARAMETER(ImageInfo);

	CWSTR		path(FullImageName);

	if (ImageInfo->SystemModeImage) {
		//	커널 드라이버가 올라올 때는 ProcessId = 0 이다.
		return;
	}

	PEPROCESS	pProcess	= NULL;

	if( NT_FAILED(PsLookupProcessByProcessId(ProcessId, &pProcess)))	return;

	if( FullImageName ) {
		PCWSTR	pName	= wcsrchr(path, L'\\');
		if( pName ) {
			//__log("%s %ws", __FUNCTION__, pName);
		}
		//SE_SIGNING_LEVEL;
		/*
		__log("%s %ws",	__FUNCTION__, pName? pName + 1 : path);
		__log("  %wZ",	FullImageName);
		__log("  ImageBase           %p", ImageInfo->ImageBase);
		__log("  ImageSize           %d", (int)ImageInfo->ImageSize);
		__log("  ImageSignatureType  %d", ImageInfo->ImageSignatureType);
		__log("  ImageSignatureLevel %d", ImageInfo->ImageSignatureLevel);
		*/

		PYFILTER_MESSAGE	pMsg = NULL;
		pMsg = (PYFILTER_MESSAGE)CMemory::Allocate(NonPagedPoolNx, sizeof(YFILTER_MESSAGE), TAG_PROCESS);
		if (pMsg) {
			RtlZeroMemory(pMsg, sizeof(YFILTER_MESSAGE));
			RtlStringCbCopyUnicodeString(pMsg->data.szCommand, sizeof(pMsg->data.szCommand), FullImageName);
			if (ProcessTable()->IsExisting(ProcessId, pMsg, [](
				IN bool					bCreationSaved,	//	생성이 감지되어 저장된 정보 여부
				IN PPROCESS_ENTRY		pEntry,			//	대상 엔트리 
				IN PVOID				pCallbackPtr
				) {
					UNREFERENCED_PARAMETER(bCreationSaved);
					PYFILTER_MESSAGE	pMsg	= (PYFILTER_MESSAGE)pCallbackPtr;
					RtlStringCbCopyUnicodeString(pMsg->data.szPath, sizeof(pMsg->data.szPath), &pEntry->ProcPath);
					pMsg->data.PUID	= pEntry->PUID;
			})) {
				//	네 존재합니다. 
			}
			else {
				__log("%s ProcessId %d is not found.", __FUNCTION__, ProcessId);
				__log("  FulLImageName:%wZ", FullImageName);
				//	이 프로세스는 생성시점 드라이버가 실행 중이지 않아 
				//	해당 프로세스 정보를 가지고 있지 않다. 
				//	[TODO]
				//AddEarlyProcess(ProcessId);
				PUNICODE_STRING	pImageFileName = NULL;
				if (NT_SUCCESS(GetProcessImagePathByProcessId(ProcessId, &pImageFileName)))
				{
					KERNEL_USER_TIMES	times;
					PROCUID				PUID;
					GetProcessTimes(ProcessId, &times);
					RtlStringCbCopyUnicodeString(pMsg->data.szPath, sizeof(pMsg->data.szPath), pImageFileName);
					GetProcGuid(false, ProcessId, NULL, pImageFileName, &times.CreateTime,
						&pMsg->data.ProcGuid, &PUID);
					pMsg->data.PUID	= PUID;
					CMemory::Free(pImageFileName);
				}
			}
			pMsg->header.mode				= YFilter::Message::Mode::Event;
			pMsg->header.category			= YFilter::Message::Category::Module;
			pMsg->header.wSize				= sizeof(YFILTER_MESSAGE);
			pMsg->header.wRevision			= 0;

			pMsg->data.subType				= YFilter::Message::SubType::ModuleLoad;
			pMsg->data.bCreationSaved		= true;
			pMsg->data.PID					= (DWORD)ProcessId;

			pMsg->data.pBaseAddress			= (ULONG_PTR)ImageInfo->ImageBase;
			pMsg->data.pImageSize			= ImageInfo->ImageSize;
			pMsg->data.ImageProperties.Property	= ImageInfo->Properties;

			if (MessageThreadPool()->Push(__FUNCTION__,
				YFilter::Message::Mode::Event,
				YFilter::Message::Category::Module,
				pMsg, sizeof(YFILTER_MESSAGE),
				false))
			{
				//	pMsg는 SendMessage 성공 후 해제될 것이다. 
				MessageThreadPool()->Alert(YFilter::Message::Category::Module);
			}
			else {
				CMemory::Free(pMsg);
			}
		}
	}
	else {
		__log("%s FullImageName is null.", __FUNCTION__);
	}
	if( pProcess )	ObDereferenceObject(pProcess);
}
bool		StartModuleFilter()
{
	__log("%s", __FUNCTION__);
	NTSTATUS	status = STATUS_SUCCESS;
	if (NULL == Config())	return false;

	__try
	{
		if( false == Config()->bModuleNotifyCallback ) {
			status	= PsSetLoadImageNotifyRoutine(LoadImageNotifyRoutine);
			if (NT_SUCCESS(status)) {
				Config()->bModuleNotifyCallback = true;
				__log("%s succeeded.", __FUNCTION__);
			}
			else {
				Config()->bModuleNotifyCallback = false;
				__log("%s PsSetLoadImageNotifyRoutine() failed. status=%08x", __FUNCTION__, status);
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