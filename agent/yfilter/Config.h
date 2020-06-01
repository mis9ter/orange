#pragma once

typedef NTSTATUS(NTAPI* FN_ZwQueryInformationProcess)(HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);
typedef NTSTATUS(NTAPI* FN_ZwTerminateProcess)(HANDLE ProcessHandle, NTSTATUS ExitStatus);
typedef NTSTATUS(NTAPI* FN_ObRegisterCallbacks)(POB_CALLBACK_REGISTRATION CallbackRegistration, PVOID *RegistrationHandle);
typedef void	(NTAPI* FN_ObUnRegisterCallbacks)(PVOID RegistrationHandle);
typedef NTSTATUS(NTAPI* FN_NtQueryInformationThread)(
	HANDLE          ThreadHandle,
	THREADINFOCLASS ThreadInformationClass,
	PVOID           ThreadInformation,
	ULONG           ThreadInformationLength,
	PULONG          ReturnLength
	);

typedef struct FLT_CLIENT_PORT
{
	KSPIN_LOCK						lock;
	HANDLE							hProcess;
	PFLT_PORT						pPort;
} FLT_CLIENT_PORT, *PFLT_CLIENT_PORT;

typedef struct CONFIG
{
	bool							bRun;
	size_t							dbSize;
	bool							bInitialized;
	PFLT_FILTER						pFilter;
	RTL_OSVERSIONINFOEXW			version;
	PDEVICE_OBJECT					pDeviceObject;
	struct {
		PFLT_PORT					pCommand;
		PFLT_PORT					pEvent;
	} server;
	struct
	{
		//	포트별 Connect/Disconnect/Send/Recv 동기화 필요
		FLT_CLIENT_PORT				command;
		FLT_CLIENT_PORT				event;
	} client;

	UNICODE_STRING					registry;
	UNICODE_STRING					name;
	UNICODE_STRING					deviceName;
	UNICODE_STRING					dosDeviceName;
	UNICODE_STRING					imagePath;
	UNICODE_STRING					systemRootPath;

	volatile PVOID					pObCallbackHandle;
	bool							bProcessNotifyCallback;
	FN_ZwQueryInformationProcess	pZwQueryInformationProcess;
	FN_ZwTerminateProcess			pZwTerminateProcess;
	FN_ObRegisterCallbacks			pObRegisterCallbacks;
	FN_ObUnRegisterCallbacks		pObUnRegisterCallbacks;

} CONFIG;


CONFIG *	Config();
PVOID		GetProcAddress(IN PWSTR pName);
bool		CreateConfig(IN PUNICODE_STRING pRegistryPath);
void		DestroyConfig();