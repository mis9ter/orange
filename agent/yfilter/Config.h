#pragma once

typedef NTSTATUS(NTAPI* FN_ZwQueryInformationProcess)(HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);
typedef NTSTATUS(NTAPI* FN_ZwTerminateProcess)(HANDLE ProcessHandle, NTSTATUS ExitStatus);
typedef NTSTATUS(NTAPI* FN_ObRegisterCallbacks)(POB_CALLBACK_REGISTRATION CallbackRegistration, PVOID *RegistrationHandle);
typedef void	(NTAPI* FN_ObUnRegisterCallbacks)(PVOID RegistrationHandle);
typedef NTSTATUS(NTAPI* FN_NtQueryInformationThread)
(
	HANDLE          ThreadHandle,
	THREADINFOCLASS ThreadInformationClass,
	PVOID           ThreadInformation,
	ULONG           ThreadInformationLength,
	PULONG          ReturnLength
	);

typedef enum _SYSTEM_INFORMATION_CLASS {
	SystemBasicInformation = 0,
	SystemPerformanceInformation = 2,
	SystemTimeOfDayInformation = 3,
	SystemProcessInformation = 5,
	SystemProcessorPerformanceInformation = 8,
	SystemInterruptInformation = 23,
	SystemExceptionInformation = 33,
	SystemRegistryQuotaInformation = 37,
	SystemLookasideInformation = 45,
	SystemCodeIntegrityInformation = 103,
	SystemPolicyInformation = 134,
} SYSTEM_INFORMATION_CLASS;

typedef	NTSTATUS(NTAPI * FN_ZwQuerySystemInformation)
(
	IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
	OUT PVOID                   SystemInformation,
	IN ULONG                    SystemInformationLength,
	OUT PULONG                  ReturnLength
);

/*
typedef enum _PSCREATETHREADNOTIFYTYPE {
	PsCreateThreadNotifyNonSystem,
	PsCreateThreadNotifySubsystems
} PSCREATETHREADNOTIFYTYPE;
*/
typedef	NTSTATUS(NTAPI * FN_ZwQueryInformationThread)
(
	_In_      HANDLE          ThreadHandle,
	_In_      THREADINFOCLASS ThreadInformationClass,
	_In_      PVOID           ThreadInformation,
	_In_      ULONG           ThreadInformationLength,
	_Out_opt_ PULONG          ReturnLength
);
// windows-internals-book:"Chapter 5"
typedef enum _PS_CREATE_STATE
{
	PsCreateInitialState,
	PsCreateFailOnFileOpen,
	PsCreateFailOnSectionCreate,
	PsCreateFailExeFormat,
	PsCreateFailMachineMismatch,
	PsCreateFailExeName, // Debugger specified
	PsCreateSuccess,
	PsCreateMaximumStates
} PS_CREATE_STATE;

typedef struct _PS_CREATE_INFO
{
	SIZE_T Size;
	PS_CREATE_STATE State;
	union
	{
		// PsCreateInitialState
		struct
		{
			union
			{
				ULONG InitFlags;
				struct
				{
					UCHAR WriteOutputOnExit : 1;
					UCHAR DetectManifest : 1;
					UCHAR IFEOSkipDebugger : 1;
					UCHAR IFEODoNotPropagateKeyState : 1;
					UCHAR SpareBits1 : 4;
					UCHAR SpareBits2 : 8;
					USHORT ProhibitedImageCharacteristics : 16;
				} Flags;
			};
			ACCESS_MASK AdditionalFileAccess;
		} InitState;

		// PsCreateFailOnSectionCreate
		struct
		{
			HANDLE FileHandle;
		} FailSection;

		// PsCreateFailExeFormat
		struct
		{
			USHORT DllCharacteristics;
		} ExeFormat;

		// PsCreateFailExeName
		struct
		{
			HANDLE IFEOKey;
		} ExeName;

		// PsCreateSuccess
		struct
		{
			union
			{
				ULONG OutputFlags;
				struct
				{
					UCHAR ProtectedProcess : 1;
					UCHAR AddressSpaceOverride : 1;
					UCHAR DevOverrideEnabled : 1; // from Image File Execution Options
					UCHAR ManifestDetected : 1;
					UCHAR ProtectedProcessLight : 1;
					UCHAR SpareBits1 : 3;
					UCHAR SpareBits2 : 8;
					USHORT SpareBits3 : 16;
				} Flags;
			};
			HANDLE FileHandle;
			HANDLE SectionHandle;
			ULONGLONG UserProcessParametersNative;
			ULONG UserProcessParametersWow64;
			ULONG CurrentParameterFlags;
			ULONGLONG PebAddressNative;
			ULONG PebAddressWow64;
			ULONGLONG ManifestAddress;
			ULONG ManifestSize;
		} SuccessState;
	};
} PS_CREATE_INFO, * PPS_CREATE_INFO;

typedef	VOID(*PCREATE_THREAD_NOTIFY_ROUTINE_EX)
(
	_Inout_ PEPROCESS Process,
	_In_ HANDLE ProcessId,
	_Inout_opt_ PPS_CREATE_INFO CreateInfo
);
typedef	NTSTATUS(NTAPI * FN_PsSetCreateThreadNotifyRoutineEx)
(
	PCREATE_THREAD_NOTIFY_ROUTINE_EX, BOOLEAN
);

typedef HMODULE (*FN_LoadLibraryA)
(
	LPCSTR lpLibFileName
);
typedef HMODULE (*FN_LoadLibraryW)
(
	LPCWSTR lpLibFileName
);

typedef struct FLT_CLIENT_PORT
{
	KSPIN_LOCK						lock;
	HANDLE							hProcess;
	PFLT_PORT						pPort;
	wchar_t							szName[32];
} FLT_CLIENT_PORT, *PFLT_CLIENT_PORT;
typedef struct FLT_SERVER_PORT {
	PFLT_PORT						pPort;
	wchar_t							szName[32];
} FLT_SERVER_PORT, *PFLT_SERVER_PORT;
typedef struct CONFIG
{
	bool							bRun;
	size_t							dbSize;
	bool							bInitialized;
	PFLT_FILTER						pFilter;
	RTL_OSVERSIONINFOEXW			version;
	PDEVICE_OBJECT					pDeviceObject;
	struct {
		FLT_SERVER_PORT				command;
		FLT_SERVER_PORT				event;
	} server;
	struct
	{
		//	포트별 Connect/Disconnect/Send/Recv 동기화 필요
		FLT_CLIENT_PORT				command;
		FLT_CLIENT_PORT				event;
	} client;

	DWORD							bootId;

	UNICODE_STRING					registry;
	UNICODE_STRING					name;
	UNICODE_STRING					deviceName;
	UNICODE_STRING					dosDeviceName;
	UNICODE_STRING					imagePath;
	UNICODE_STRING					systemRootPath;
	UNICODE_STRING					machineGuid;

	volatile PVOID					pObCallbackHandle;
	bool							bProcessNotifyCallback;
	bool							bModuleNotifyCallback;
	bool							bThreadNotifyCallback;
	bool							bThreadNotifyCallbackEx;
	FN_ZwQueryInformationProcess	pZwQueryInformationProcess;
	FN_ZwTerminateProcess			pZwTerminateProcess;
	FN_ObRegisterCallbacks			pObRegisterCallbacks;
	FN_ObUnRegisterCallbacks		pObUnRegisterCallbacks;
	FN_NtQueryInformationThread		pNtQueryInformationThread;
	FN_PsSetCreateThreadNotifyRoutineEx		pPsSetCreateThreadNotifyRoutineEx;
	FN_ZwQueryInformationThread		pZwQueryInformationThread;
	FN_ZwQuerySystemInformation		pZwQuerySystemInformation;
	FN_LoadLibraryA					pLoadLibraryA;
	FN_LoadLibraryW					pLoadLibraryW;

} CONFIG;


CONFIG *	Config();
PVOID		GetProcAddress(IN PWSTR pName);
bool		CreateConfig(IN PUNICODE_STRING pRegistryPath);
void		DestroyConfig();