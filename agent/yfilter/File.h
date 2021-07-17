#include "yfilter.h"

typedef struct Y_FILE_CONTEXT {
	LARGE_INTEGER		times[2];
	HANDLE				PID;
	HANDLE				TID;
	ULONG				SID;
} Y_FILE_CONTEXT, *PY_FILE_CONTEXT;

CFileTable *			FileTable();
void					CreateFileTable();
void					DestroyFileTable();
PY_FILE_CONTEXT			CreateFileContext();
void					ReleaseFileContext(PY_FILE_CONTEXT p);
PY_STREAMHANDLE_CONTEXT	CreateStreamHandle();
void					DeleteStreamHandleCallback(PY_STREAMHANDLE_CONTEXT p);

EXTERN_C_START

void					CreateFileMessage(
	PY_STREAMHANDLE_CONTEXT	p,
	PY_FILE_MESSAGE			*pOut
);

EXTERN_C_END

class CFile
{
public:

	static	bool				IsNegligibleIO(PETHREAD CONST Thread)
	{
		//	무시 가능한 녀석들
		//	필터링 중지거나 커널 쓰레드인 경우 체크하지 않음 
		//	(추후 커널레벨에서의 방어가 필요한 경우 IoIsSystemThread 처리 부분 제거)
		if (
			NULL == Thread						||
			NULL == Config()					||
			NULL == Config()->pFilter			||
			IoIsSystemThread(Thread)			||		//	시스템 스레드인 경우
			KernelMode == ExGetPreviousMode()	||		//	The ExGetPreviousMode routine returns the previous processor mode for the current thread.
			KeGetCurrentIrql() != PASSIVE_LEVEL ||
			IoGetTopLevelIrp() != NULL
			)
		{
			//__log("Negligible IO");
			return true;
		}
		return false;
	}
	static	bool				IsWriteIO(
		ACCESS_MASK     accessMask,
		ULONG           disposition,
		ULONG			createOption
	)
	{
		if( disposition != FILE_OPEN )	return true;
		if ((accessMask & FILE_WRITE_DATA) || 
			(accessMask & FILE_WRITE_ATTRIBUTES) || 
			(accessMask & FILE_WRITE_EA) || 
			(accessMask & FILE_APPEND_DATA) ||
			(accessMask & WRITE_OWNER) || 
			(accessMask & WRITE_DAC) || 
			(accessMask & DELETE) 
			)
			return true;
		if( createOption & FILE_DELETE_ON_CLOSE)
			return true;
		return false;
	}
	static	PFLT_FILE_NAME_INFORMATION	FltGetFileName(
		PFLT_CALLBACK_DATA		pCallbackData,
		PCFLT_RELATED_OBJECTS	pFltObjects,
		bool *					pbIsDirectory
	)
	{
		/*
		FltGetFileNameInformation
		https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/fltkernel/nf-fltkernel-fltgetfilenameinformation

		FLT_FILE_NAME_NORMALIZED
		The FileNameInformation parameter receives the address of a structure containing the normalized name for the file.

		FLT_FILE_NAME_OPENED
		The FileNameInformation parameter receives the address of a structure containing the name
		that was used when the file was opened.

		FLT_FILE_NAME_SHORT
		The FileNameInformation parameter receives the address of a structure containing the short (8.3) name for the file.
		The short name consists of up to 8 characters, followed immediately by a period and up to 3 more characters.
		The short name for a file does not include the volume name, directory path, or stream name.
		Not valid in the pre-create path.

		FLT_FILE_NAME_QUERY_DEFAULT
		If it is not currently safe to query the file system for the file name, FltGetFileNameInformation does nothing.
		Otherwise, FltGetFileNameInformation queries the Filter Manager's name cache for the file name information.
		If the name is not found in the cache, FltGetFileNameInformation queries the file system and caches the result.

		FLT_FILE_NAME_QUERY_CACHE_ONLY
		FltGetFileNameInformation queries the Filter Manager's name cache for the file name information.
		FltGetFileNameInformation does not query the file system.

		FLT_FILE_NAME_QUERY_FILESYSTEM_ONLY
		FltGetFileNameInformation queries the file system for the file name information.
		FltGetFileNameInformation does not query the Filter Manager's name cache,
		and does not cache the result of the file system query.

		FLT_FILE_NAME_REQUEST_FROM_CURRENT_PROVIDER
		A name provider minifilter can use this flag to specify that a name query request should be redirected to itself
		(the name provider minifilter) rather than being satisfied by the name providers lower in the stack.

		FLT_FILE_NAME_ALLOW_QUERY_ON_REPARSE
		A name provider minifilter can use this flag to specify
		that it is safe to query the name in the post-create path even if STATUS_REPARSE was returned.
		It is the caller's responsibility to ensure that the FileObject->FileName field was not changed.
		Do not use this flag with mount points or symbolic link reparse points.
		This flag is available on Microsoft Windows Server 2003 SP1 and later.
		This flag is also available on Windows 2000 SP4 with Update Rollup 1 and later.
		*/
		if (KeGetCurrentIrql() > PASSIVE_LEVEL || IoGetTopLevelIrp() != NULL) {
			/*
			IoGetTopLevelIrp can return NULL, an arbitrary file-system-specific value
			(such as a pointer to the current IRP), or one of the flags listed in the following table.

			FSRTL_FSP_TOP_LEVEL_IRP			This is a recursive call.
			FSRTL_CACHE_TOP_LEVEL_IRP		The cache manager is the top-level component for the current thread.
			FSRTL_MOD_WRITE_TOP_LEVEL_IRP	The modified page writer is the top-level component for the current thread.
			FSRTL_FAST_IO_TOP_LEVEL_IRP		The cache manager is the top-level component for the current thread,
			and the current thread is in a fast I/O path.
			*/
			//__dlog("%-32s IRQL=%d", __func__, KeGetCurrentIrql());
			return NULL;
		}
		/*
		FILE_OBJECT Names in IRP_MJ_CREATE
		http://fsfilters.blogspot.com/2011/09/fileobject-names-in-irpmjcreate.html
		*/
		if (NULL == pCallbackData || NULL == pFltObjects || NULL == pFltObjects->FileObject ||
			NULL == pFltObjects->FileObject->FileName.Buffer)
		{
			//__dlog("%-32s NULL", __func__);
			return NULL;
		}
		if (pFltObjects->FileObject->FileName.Length <= 6 ||
			(
				pFltObjects->FileObject->FileName.Buffer[0] == L'\\'	&&
				pFltObjects->FileObject->FileName.Buffer[1] == L'$'		&&
				pFltObjects->FileObject->FileName.Buffer[2] != L'R'
				)
			)
		{
			//	NTFS Meta 정보 관련 오퍼레이션
			//__log("%-32s %wZ", __func__, &pFltObjects->FileObject->FileName);
			return NULL;
		}

		NTSTATUS	status = STATUS_UNSUCCESSFUL;
		BOOLEAN		bIsDirectory = false;
		FltIsDirectory(pFltObjects->FileObject, pFltObjects->Instance, &bIsDirectory);
		if (pbIsDirectory)	*pbIsDirectory = bIsDirectory ? true : false;

		PFLT_FILE_NAME_INFORMATION pFileNameInfo = NULL;
		__try
		{
			/*
			status = FltGetFileNameInformation(
			pCallbackData,
			FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT,
			&pFileNameInfo
			);
			*/
			if (NT_FAILED(status) || NULL == pFileNameInfo)
			{
				status = FltGetFileNameInformation(
					pCallbackData,
					FLT_FILE_NAME_OPENED | FLT_FILE_NAME_QUERY_ALWAYS_ALLOW_CACHE_LOOKUP,
					&pFileNameInfo
				);
				if (NT_FAILED(status) || NULL == pFileNameInfo)
				{
					//	STATUS_INVALID_PARAMETER 오류로 실패되는 경우가 많은데
					//	주로 어떤 경우일까요?
					__dlog("%s FltGetFileNameInformation() failed. status=%08x",
						__FUNCTION__, status);
					__leave;
				}
			}
			status = FltParseFileNameInformation(pFileNameInfo);
			if (NT_FAILED(status)) {
				__log("%s FltParseFileNameInformation() failed. status=%08x", __FUNCTION__, status);
				__leave;
			}
			if ((pFileNameInfo->Name.Buffer[0] == L'\\') &&
				(pFileNameInfo->Name.Buffer[1] == L'$') &&
				(pFileNameInfo->Name.Buffer[2] != L'R')
				)
			{
				//	\$Extend\$Quota:$Q:$INDEX_ALLOCATION
				//	이와 같은 파일명이 등장할 수 있다. 
				//	이건 NTFS metadata
				//	https://superuser.com/questions/1092378/what-is-c-extend-reparserindex-allocation-in-win7
				//	http://ntfs.com/ntfs-system-files.htm
				//	https://en.wikipedia.org/wiki/NTFS_reparse_point
				//	그런데 \$R 로 시작되는 것은 무엇인가?
				//__log("%s(2) %wZ", __FUNCTION__, &pFileNameInfo->Name);
				__leave;
			}
			status = STATUS_SUCCESS;
		}
		_finally
		{
			if (!NT_SUCCESS(status))
			{
				if (pFileNameInfo != NULL)
				{
					FltReleaseFileNameInformation(pFileNameInfo);
				}
				pFileNameInfo = NULL;
			}
		}
		return pFileNameInfo;
	}
	static	BOOLEAN	FLTAPI		IsWriteIONew
	(
		ACCESS_MASK     accessMask,
		ULONG           disposition,
		ULONG			createOption
	)
	{
		BOOLEAN bWriteIo = TRUE;
		__try
		{
			if (disposition != FILE_OPEN)	// userlevel의 OPEN_EXISTING 에 해당.
				__leave;
			if ((accessMask & FILE_WRITE_DATA) || (accessMask & FILE_WRITE_ATTRIBUTES) || (accessMask & FILE_WRITE_EA) || (accessMask & FILE_APPEND_DATA))
				__leave;
			if ((accessMask & WRITE_OWNER) || (accessMask & WRITE_DAC))
				__leave;
			if (createOption & FILE_DELETE_ON_CLOSE)
				__leave;
			bWriteIo = FALSE;
		}
		__finally {


		}
		return bWriteIo;
	}
	static	BOOLEAN	FLTAPI		IsReadIO(
		ACCESS_MASK     accessMask,
		ULONG           disposition,
		ULONG			createOption
	)
	{
		UNREFERENCED_PARAMETER(createOption);
		BOOLEAN bReadIo = FALSE;
		__try
		{
			if (disposition != FILE_OPEN)	// userlevel의 OPEN_EXISTING 에 해당.
				__leave;
			if (accessMask & FILE_READ_DATA)
				bReadIo = TRUE;
		}
		__finally {

		}
		return bReadIo;
	}
};