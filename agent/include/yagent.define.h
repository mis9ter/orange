#pragma once

#include <guiddef.h>
#ifndef UUID
typedef GUID UUID;
#endif UUID


#define AGENT_SERVICE_NAME		L"xagent"
#define AGENT_WINDOW_NAME		AGENT_SERVICE_NAME
#define AGENT_DISPLAY_NAME		L"by oragneworks"
#define AGENT_LOG_NAME			L"yagent.log"
#define	AGENT_PATH_SIZE			1024
#define AGENT_NAME_SIZE			64
#define DRIVER_SERVICE_NAME		L"yfilter"
#define DRIVER_FILE_NAME		L"yfilter.sys"
#define DRIVER_COMMAND_PORT		L"\\yfilter_command"
#define DRIVER_EVENT_PORT		L"\\yfilter_event"
#define DRIVER_DISPLAY_NAME		L"by orangeworks"
#define DRIVER_INSTANCE_NAME	L"orange filter"
#define DRIVER_ALTITUDE			(385200)			// 미니필터 고도 (추후 등록시 필요한 경우 수정 필요)
#define DRIVER_STRING_ALTITUDE	L"385200"			// 미니필터 고도 (추후 등록시 필요한 경우 수정 필요)
#define DRIVER_MAX_MESSAGE_SIZE			4096

#define GUID_STRING_SIZE		36
#define NANOSECONDS(nanos) (((signed __int64)(nanos)) / 100L)
#define MICROSECONDS(micros) (((signed __int64)(micros)) * NANOSECONDS(1000L))
#define MILLISECONDS(milli) (((signed __int64)(milli)) * MICROSECONDS(1000L))
#define SECONDS(seconds) (((signed __int64)(seconds)) * MILLISECONDS(1000L))

#define DRIVER_MESSAGE_TIMEOUT			(3000000000)

#define SAFEBOOT_REG_MINIMAL	L"SYSTEM\\CurrentControlSet\\Control\\SafeBoot\\Minimal"
#define SAFEBOOT_REG_NETWORK	L"SYSTEM\\CurrentControlSet\\Control\\SafeBoot\\Network"
#define TEXTLINE				"--------------------------------------------------------------------------------"

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
		};
		enum SubType {
			ProcessStart,
			ProcessStop,
			ThreadStart,
			ThreadStop
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

typedef struct YFILTER_DATA {
	YFilter::Message::SubType	subType;

	union {
		bool					bCreationSaved;				//	process
	};
	DWORD						dwProcessId;				//	all
	DWORD						dwThreadId;					//	all
	union {
		DWORD					dwParentProcessId;			//	process
		DWORD					dwCreateProcessId;			//	thread
	};
	union {
		ULONG_PTR				pBaseAddress;				//	module
		PVOID					pStartAddress;				//	thread
	};

	wchar_t						szPath[AGENT_PATH_SIZE];
	wchar_t						szCommand[AGENT_PATH_SIZE];

#pragma pack(pop)
	UUID						uuid;						//	process

#pragma pack(push, 1)
} YFILTER_DATA, *PYFILTER_DATA;
typedef struct YFILTER_MESSAGE {
	YFILTER_HEADER	header;
	YFILTER_DATA	data;
} YFILTER_MESSAGE, *PYFILTER_MESSAGE;

typedef struct YFILTER_REPLY
{
	bool					bRet;
} FILTER_REPLY;
#pragma pack(pop)