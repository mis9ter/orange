#include "yfilter.h"
//***************************************************************************************
//*	System Root Driver
//*
//*		This driver prints the system root path and drive to the debugger.  
//*
//*	Jun	2004 - DriverEntry (www.DriverEntry.com)
//***************************************************************************************
// Software is provided "as is", without warranty or guarantee of any kind. The use of 
// this software is at your own risk. We take no responsibly for any damage that may be 
// caused through its use. DriverEntry source code may not be used in any product without 
// written consent. DriverEntry source code or software may not distributed in any form 
// without written consent. All enquiries should be made to info@driverenrty.com. 
//
// THE ENTIRE RISK FROM THE USE OF THIS SOFTWARE REMAINS WITH YOU. 
//***************************************************************************************

//#######################################################################################
//# D E F I N E S
//#######################################################################################

#define SYSTEM_ROOT		L"\\SystemRoot"

//***************************************************************************************
//* NAME:			GetSymbolicLink
//*
//* DESCRIPTION:	Given a symbolic link name this routine returns a string with the 
//*					links destination and a handle to the open link name.
//*					
//*	PARAMETERS:		SymbolicLinkName		IN
//*					SymbolicLink			OUT
//*					LinkHandle				OUT
//*
//*	IRQL:			IRQL_PASSIVE_LEVEL.
//*
//*	RETURNS:		STATUS_SUCCESS
//*					STATUS_UNSUCCESSFUL
//*
//* NOTE:			Caller must free SymbolicLink->Buffer AND close the LinkHandle 
//*					after a successful call to this routine.
//***************************************************************************************
static
NTSTATUS
GetSymbolicLink
(
	IN		PUNICODE_STRING		SymbolicLinkName,
	OUT		PUNICODE_STRING		SymbolicLink,
	OUT		PHANDLE				LinkHandle
)
{
	NTSTATUS			status;
	NTSTATUS			returnStatus;
	OBJECT_ATTRIBUTES	oa;
	UNICODE_STRING		tmpSymbolicLink;
	HANDLE				tmpLinkHandle;
	ULONG				symbolicLinkLength;

	returnStatus = STATUS_UNSUCCESSFUL;

	//
	// Open and query the symbolic link
	//
	__log("%s %wZ", __FUNCTION__, SymbolicLinkName);
	InitializeObjectAttributes
	(
		&oa,
		SymbolicLinkName,
		OBJ_KERNEL_HANDLE,
		NULL,
		NULL
	);

	status = ZwOpenSymbolicLinkObject
	(
		&tmpLinkHandle,
		GENERIC_READ,
		&oa
	);
	if (NT_SUCCESS(status))
	{
		//
		// Get the size of the symbolic link string
		//
		symbolicLinkLength = 0;
		tmpSymbolicLink.Length = 0;
		tmpSymbolicLink.MaximumLength = 0;

		status = ZwQuerySymbolicLinkObject
		(
			tmpLinkHandle,
			&tmpSymbolicLink,
			&symbolicLinkLength
		);
		if (STATUS_BUFFER_TOO_SMALL == status && symbolicLinkLength > 0)
		{
			//
			// Allocate the memory and get the ZwQuerySymbolicLinkObject
			//
			tmpSymbolicLink.Buffer = (PWCH)ExAllocatePool(NonPagedPool, symbolicLinkLength);
			tmpSymbolicLink.Length = 0;
			tmpSymbolicLink.MaximumLength = (USHORT)symbolicLinkLength;

			status = ZwQuerySymbolicLinkObject
			(
				tmpLinkHandle,
				&tmpSymbolicLink,
				&symbolicLinkLength
			);

			if (STATUS_SUCCESS == status)
			{
				SymbolicLink->Buffer = tmpSymbolicLink.Buffer;
				SymbolicLink->Length = tmpSymbolicLink.Length;
				*LinkHandle = tmpLinkHandle;
				SymbolicLink->MaximumLength = tmpSymbolicLink.MaximumLength;
				returnStatus = STATUS_SUCCESS;
			}
			else
			{
				ExFreePool(tmpSymbolicLink.Buffer);
			}
		}
		else {
			__log("%s ZwQuerySymbolicLinkObject() failed. status=%08x", __FUNCTION__, status);
		}
	} 
	else {
		//	STATUS_OBJECT_TYPE_MISMATCH
		__log("%s ZwOpenSymbolicLinkObject() failed. status=%08x", __FUNCTION__, status);
	}
	return status;
}

//***************************************************************************************
//* NAME:			ExtractDriveString
//*
//* DESCRIPTION:	Extracts the drive from a string.  Adds a NULL termination and 
//*					adjusts the length of the source string.
//*					
//*	PARAMETERS:		Source				IN OUT
//*
//*	IRQL:			IRQL_PASSIVE_LEVEL.
//*
//*	RETURNS:		STATUS_SUCCESS
//*					STATUS_UNSUCCESSFUL
//***************************************************************************************
static
NTSTATUS
ExtractDriveString
(
	IN OUT	PUNICODE_STRING		Source
)
{
	NTSTATUS	status;
	ULONG		i;
	ULONG		numSlashes;

	status = STATUS_UNSUCCESSFUL;
	i = 0;
	numSlashes = 0;

	while (((i * 2) < Source->Length) && (4 != numSlashes))
	{
		if (L'\\' == Source->Buffer[i])
		{
			numSlashes++;
		}
		i++;
	}

	__log("%s %wZ %d", __FUNCTION__, Source, numSlashes);

	if ((4 == numSlashes) && (i > 1))
	{
		i--;
		Source->Buffer[i] = L'\0';
		Source->Length = (USHORT)i * 2;
	}
	status = STATUS_SUCCESS;

	return status;
}

//***************************************************************************************
//* NAME:			GetSystemRootPath
//*
//* DESCRIPTION:	On success this routine allocates and copies the system root path 
//*					(ie C:\Windows) into the SystemRootPath parameter
//*					
//*	PARAMETERS:		SystemRootPath				out
//*
//*	IRQL:			IRQL_PASSIVE_LEVEL.
//*
//*	RETURNS:		STATUS_SUCCESS
//*					STATUS_UNSUCCESSFUL
//*
//* NOTE:			Caller must free SystemRootPath->Buffer after a successful call to 
//*					this routine.
//***************************************************************************************
NTSTATUS
GetSystemRootPath
(
	OUT		PUNICODE_STRING		SystemRootPath
)
{
	NTSTATUS			status;
	NTSTATUS			returnStatus;
	UNICODE_STRING		systemRootName;
	UNICODE_STRING		systemRootSymbolicLink1;
	UNICODE_STRING		systemRootSymbolicLink2;
	UNICODE_STRING		systemDosRootPath;
	PDEVICE_OBJECT		deviceObject;
	PFILE_OBJECT		fileObject;
	HANDLE				linkHandle;
	ULONG				fullPathLength;

	returnStatus = STATUS_UNSUCCESSFUL;

	RtlInitUnicodeString
	(
		&systemRootName,
		SYSTEM_ROOT
	);

	//
	// Get the full path for the system root directory
	//
	status = GetSymbolicLink
	(
		&systemRootName,
		&systemRootSymbolicLink1,
		&linkHandle
	);

	if (STATUS_SUCCESS == status)
	{
		//
		// At this stage we have the full path but its in the form:
		// \Device\Harddisk0\Partition1\WINDOWS lets try to get the symoblic name for 
		// this drive so it looks more like c:\WINDOWS.
		//
		//__log("Full System Root Path: %ws", systemRootSymbolicLink1.Buffer);
		fullPathLength = systemRootSymbolicLink1.Length;
		//
		// Remove the path so we can query the drive letter
		//
		status = ExtractDriveString(&systemRootSymbolicLink1);
		if (NT_FAILED(status)) {
			__log("%s ExtractDriveString() failed. status=%08x", __FUNCTION__, status);
			return status;
		}
		if (STATUS_SUCCESS == status)
		{
			//
			// We've added a NULL termination character so we must reflect that in the 
			// total length.
			//
			fullPathLength = fullPathLength - 2;
			ZwClose(linkHandle);
			//
			// Query the drive letter
			//
			status = GetSymbolicLink
			(
				&systemRootSymbolicLink1,
				&systemRootSymbolicLink2,
				&linkHandle
			);
			if (STATUS_SUCCESS == status)
			{
				status = IoGetDeviceObjectPointer
				(
					&systemRootSymbolicLink2,
					SYNCHRONIZE | FILE_ANY_ACCESS,
					&fileObject,
					&deviceObject
				);

				if (STATUS_SUCCESS == status)
				{
					ObReferenceObject(deviceObject);

					//
					// Get the dos name for the drive
					//
					status = RtlVolumeDeviceToDosName
					(
						deviceObject,
						&systemDosRootPath
					);

					if (STATUS_SUCCESS == status && NULL != systemDosRootPath.Buffer)
					{
						__log("%s 4", __FUNCTION__);
						SystemRootPath->Buffer = (PWCH)ExAllocatePool
						(
							NonPagedPool,
							fullPathLength
						);
						RtlZeroMemory(SystemRootPath->Buffer, fullPathLength);
						SystemRootPath->Length = 0;
						SystemRootPath->MaximumLength = (USHORT)fullPathLength;

						//
						// Drive
						//
						RtlCopyMemory
						(
							SystemRootPath->Buffer,
							systemDosRootPath.Buffer,
							systemDosRootPath.Length
						);

						//
						// Drive Slash
						//
						RtlCopyMemory
						(
							SystemRootPath->Buffer + (systemDosRootPath.Length / 2),
							L"\\",
							2
						);

						//
						// Drive Slash Directory
						//
						RtlCopyMemory
						(
							SystemRootPath->Buffer + (systemDosRootPath.Length / 2) + 1,
							systemRootSymbolicLink1.Buffer +
							(systemRootSymbolicLink1.Length / 2) + 1,
							fullPathLength - systemRootSymbolicLink1.Length
						);

						SystemRootPath->Length = (systemDosRootPath.Length + 2) +
							((USHORT)fullPathLength - systemRootSymbolicLink1.Length);

						ExFreePool(systemDosRootPath.Buffer);

						returnStatus = STATUS_SUCCESS;
					}
					else {
						__log("%s 11", __FUNCTION__);
					}
					ObDereferenceObject(deviceObject);
				}
				else {
					__log("%s IoGetDeviceObjectPointer() failed. status=%08x", __FUNCTION__, status);
				}
				ZwClose(linkHandle);
				ExFreePool(systemRootSymbolicLink2.Buffer);
			}
			else {
				__log("%s GetSymbolicLink() failed. status=%08x", __FUNCTION__, status);
			}
		}
		ExFreePool(systemRootSymbolicLink1.Buffer);
	}
	return returnStatus;
}