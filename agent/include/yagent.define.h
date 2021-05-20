#pragma once

#include <guiddef.h>
#ifndef UUID
typedef GUID UUID;
#endif UUID

#define AGENT_SERVICE_NAME		L"orange.service"
#define AGENT_WINDOW_NAME		AGENT_SERVICE_NAME
#define AGENT_DISPLAY_NAME		L"by oragneworks"
#define AGENT_SERVICE_DESC		L"orange.service"
#define AGENT_DEFAULT_LOG_NAME	L"orange.log"
#define AGENT_SERVICE_PIPE_NAME	L"\\\\.\\pipe\\{523E4858-04BA-4CB1-AE5D-6AD6C9503C16}"
#define AGENT_WEBAPP_PIPE_NAME	L"\\\\.\\pipe\\{D12BB45C-977B-4FBA-88A2-1A4F6B3D1D75}"
#define AGENT_DATA_FOLDERNAME	L"\\Orangeworks\\Orange"



#define	AGENT_PATH_SIZE			1024
#define AGENT_NAME_SIZE			64
#define AGENT_RUNLOOP_PERIOD	3000

#define DRIVER_SERVICE_NAME		L"orange.driver"
#define DRIVER_FILE_NAME		L"orange.sys"
#define DRIVER_COMMAND_PORT		L"\\orange_command"
#define DRIVER_EVENT_PORT		L"\\orange_event"
#define DRIVER_DISPLAY_NAME		L"by orangeworks"
#define DRIVER_INSTANCE_NAME	L"orange filter"
#define DRIVER_ALTITUDE			(385200)			// 미니필터 고도 (추후 등록시 필요한 경우 수정 필요)
#define DRIVER_STRING_ALTITUDE	L"385200"			// 미니필터 고도 (추후 등록시 필요한 경우 수정 필요)
#define DRIVER_MAX_MESSAGE_SIZE	4096

#define DB_EVENT_DB				L"event.db"

#define GUID_STRING_SIZE		36
#define NANOSECONDS(nanos) (((signed __int64)(nanos)) / 100L)
#define MICROSECONDS(micros) (((signed __int64)(micros)) * NANOSECONDS(1000L))
#define MILLISECONDS(milli) (((signed __int64)(milli)) * MICROSECONDS(1000L))
#define SECONDS(seconds) (((signed __int64)(seconds)) * MILLISECONDS(1000L))

#define DRIVER_MESSAGE_TIMEOUT			(3000000000)

#define SAFEBOOT_REG_MINIMAL	L"SYSTEM\\CurrentControlSet\\Control\\SafeBoot\\Minimal"
#define SAFEBOOT_REG_NETWORK	L"SYSTEM\\CurrentControlSet\\Control\\SafeBoot\\Network"
#define TEXTLINE				"--------------------------------------------------------------------------------"

#ifdef ENABLE_RTL_NUMBER_OF_V2
#define RTL_NUMBER_OF(A) RTL_NUMBER_OF_V2(A)
#else
#define RTL_NUMBER_OF(A) RTL_NUMBER_OF_V1(A)
#endif

#ifndef _PHLIB_
typedef union _PS_PROTECTION
{
	UCHAR Level;
	struct
	{
		int Type : 3;
		int Audit : 1;
		int Signer : 4;
	} Flags;
} PS_PROTECTION, * PPS_PROTECTION;

typedef enum _PS_PROTECTED_SIGNER
{
	PsProtectedSignerNone = 0,
	PsProtectedSignerAuthenticode = 1,
	PsProtectedSignerCodeGen = 2,
	PsProtectedSignerAntimalware = 3,
	PsProtectedSignerLsa = 4,
	PsProtectedSignerWindows = 5,
	PsProtectedSignerWinTcb = 6,
	PsProtectedSignerMax = 7
} PS_PROTECTED_SIGNER;

typedef enum _PS_PROTECTED_TYPE
{
	PsProtectedTypeNone = 0,
	PsProtectedTypeProtectedLight = 1,
	PsProtectedTypeProtected = 2,
	PsProtectedTypeMax = 3

} PS_PROTECTED_TYPE;
#endif

namespace YFilter
{
	namespace Message
	{
		enum Mode {
			Command,
			Event
		};
		enum Category {
			Process,
			Thread,
			Module,
			Boot,
			Count
		};
		enum SubType {
			ProcessStart,	//	지금 프로세스가 실행되었다.
			ProcessStop,	//	지금 프로세스가 종료되었다.
			ProcessStart2,	//	이전에 실행된 프로세스를 이제 알게 되었다.
			ThreadStart,
			ThreadStop,
			ModuleLoad,
		};
	};
	namespace Object
	{
		enum Mode {
			Protect,
			Allow,
			White,		//	접근 시 죽일 놈
			Gray,		//	접근 못하게 - 죽이진 않으마
			Black,		//	접근 대환영 - Allow와 같은 개념이지만 따로 관리하고 싶다.
		};
		enum Type {
			Unknown = -1,
			File,
			Process,
			Registry,
			Memory,
			End,
		};
	};
	namespace Process
	{
		enum Type {
			White,		//	내게 무슨 짓을 해도 허용할 녀석들
			Gray,		//	종료 + VM 관련 행위를 차단할 녀석들
			Black,		//	내게 무슨 짓을 해도 막아야할 녀석들
			Unknown,	//	종료는 못 하게 해야 할 녀석들
		};
		enum Context {
			Process,
			Thread
		};
	};

	inline	PCWSTR GetModeName(IN YFilter::Object::Mode mode)
	{
		YFilter::Object::Mode::Protect;
		static	PCWSTR	pNames[] = {
			L"Protect",
			L"Allow",
			L"White",
			L"Gray",
			L"Black"
		};
		static	PCWSTR	pUnknown = L"unknown";
		switch (mode)
		{
		case YFilter::Object::Mode::Protect:
		case YFilter::Object::Mode::Allow:
		case YFilter::Object::Mode::Black:
		case YFilter::Object::Mode::Gray:
		case YFilter::Object::Mode::White:
			return pNames[mode];
		}
		return pUnknown;
	}
	inline	PCWSTR GetTypeName(IN YFilter::Object::Type type)
	{
		static	PCWSTR	pNames[] = {
			L"File",
			L"Process",
			L"Registry",
			L"Memory",
		};
		static	PCWSTR	pUnknown = L"unknown";
		switch (type)
		{
		case YFilter::Object::Type::File:
		case YFilter::Object::Type::Registry:
		case YFilter::Object::Type::Process:
		case YFilter::Object::Type::Memory:
			return pNames[type];
		}
		return pUnknown;
	}
};

/////////////////////////////////////////////////////////////////////////////////////////
//	도라이버-에이전트 정보 구조체
/////////////////////////////////////////////////////////////////////////////////////////
#pragma pack(push, 1)
#define MESSAGE_MAX_SIZE		(64 * 1024)			//	커널에서 전달 예상되는 최대 크기
typedef struct YFILTER_HEADER {
	YFilter::Message::Mode		mode;				//
	YFilter::Message::Category	category;			//	
	ULONG						size;				//	MESSAGE_HEADER + 알파
} YFILTER_HEADER, *PYFILTER_HEADER;
/*
	[TODO]	2020/08/01

	생긴걸 아무리 쬐려봐도 FltSendMessage - FilterGetMessage 쌍은 정해진 크기로만
	주고 받아야 하는 걸로 보인다.

	그렇다면 한 구조체 안에 다양한 mode, category, subtype을 수용할 수 있도록 
	아껴서 잘 만들어야 한다는 이야기.

	아.. 그래서 xfilter 에서 주고 받는 구조체에 union이 많은 거구나.
	서로 다른 애들끼리는 필드를 중첩 시키려고.
*/
#if !defined(_NTDDK_) && !defined(_PHLIB_)
typedef struct _KERNEL_USER_TIMES {
	LARGE_INTEGER CreateTime;
	LARGE_INTEGER ExitTime;
	LARGE_INTEGER KernelTime;
	LARGE_INTEGER UserTime;
} KERNEL_USER_TIMES;
typedef KERNEL_USER_TIMES* PKERNEL_USER_TIMES;
#endif

#define YFILTER_COMMAND_START	0x00000001
#define YFILTER_COMMAND_STOP	0x00000000

typedef struct YFILTER_DATA {
	YFilter::Message::SubType	subType;

	union {
		bool					bCreationSaved;				//	process
	};
	DWORD						PID;						//	current process id/all
	DWORD						PPID;						//	parent process id/process
	DWORD						TID;						//	target thread id/all
	DWORD						CTID;						//	current thread id
	DWORD						CPID;						//	creator process id/thread
	DWORD						RPID;						//	related process id
	union {
		ULONG_PTR				pBaseAddress;				//	module
		ULONG_PTR				pStartAddress;				//	thread
	};
	bool						IsSystem;

	wchar_t						szPath[AGENT_PATH_SIZE];
	wchar_t						szCommand[AGENT_PATH_SIZE];
	KERNEL_USER_TIMES			times;
#pragma pack(pop)
	UUID						ProcGuid;					//	process
	UUID						PProcGuid;
#pragma pack(push, 1)

	ULONG_PTR					pImageSize;
	union {
		ULONG Property;
		struct {
			ULONG ImageAddressingMode : 8;  // Code addressing mode
			ULONG SystemModeImage : 1;  // System mode image
			ULONG ImageMappedToAllPids : 1;  // Image mapped into all processes
			ULONG ExtendedInfoPresent : 1;  // IMAGE_INFO_EX available
			ULONG MachineTypeMismatch : 1;  // Architecture type mismatch
			ULONG ImageSignatureLevel : 4;  // Signature level
			ULONG ImageSignatureType : 3;  // Signature type
			ULONG ImagePartialMap : 1;  // Nonzero if entire image is not mapped
			ULONG Reserved : 12;
		} Properties;
	} ImageProperties;
} YFILTER_DATA, *PYFILTER_DATA;
typedef struct YFILTER_MESSAGE {
	YFILTER_HEADER	header;
	YFILTER_DATA	data;
} YFILTER_MESSAGE, *PYFILTER_MESSAGE;

typedef struct YFILTER_COMMAND
{
	DWORD					dwCommand;
} YFILTER_COMMAND, * PYFILTER_COMMAND;

typedef struct YFILTER_REPLY
{
	bool					bRet;
} FILTER_REPLY;
#pragma pack(pop)