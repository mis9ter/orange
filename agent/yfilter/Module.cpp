#include "yfilter.h"
/*
Remarks
Highest-level system-profiling drivers can call PsSetLoadImageNotifyRoutine to set up their load-image notify routines (see PLOAD_IMAGE_NOTIFY_ROUTINE).

The maximum number of drivers that can be simultaneously registered to receive load-image notifications is eight. If the maximum number of load-image notify routines is already registered when a driver calls PsSetLoadImageNotifyRoutine to try to register an additional notify routine, PsSetLoadImageNotifyRoutine fails and returns STATUS_INSUFFICIENT_RESOURCES.

Notes

An update for Windows 8.1 increases the maximum number of drivers registered to receive load-image notifications from eight to 64. This update is installed as part of a cumulative update that is available through Windows Update starting on April 8, 2014. In addition, this cumulative update is available at https://support.microsoft.com/kb/2919355.
Users of Windows 7 with Service Pack 1 (SP1) can install a hotfix to increase the maximum number of drivers registered to receive load-image notifications from eight to 64. This hotfix is available at https://support.microsoft.com/kb/2922790.
*/

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
	}
	else {
		__log("%s FullImageName is null.", __FUNCTION__);
	}
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