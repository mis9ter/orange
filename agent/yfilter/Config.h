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
		KSPIN_LOCK					lock;
		struct {
			PFLT_PORT				pCommand;
			HANDLE					hProcess;
		} ;
		struct {
			PFLT_PORT				pEvent;
			HANDLE					hProcess;
		};
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