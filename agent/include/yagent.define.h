#pragma once

#define AGENT_SERVICE_NAME		L"orange"
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
#define DRIVER_MESSAGE_TIMEOUT			(30000000)

#define SAFEBOOT_REG_MINIMAL	L"SYSTEM\\CurrentControlSet\\Control\\SafeBoot\\Minimal"
#define SAFEBOOT_REG_NETWORK	L"SYSTEM\\CurrentControlSet\\Control\\SafeBoot\\Network"

namespace YFilter
{
	namespace Message
	{
		enum Category {
			Event,
			Command
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