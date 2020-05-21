#pragma once
#include <windows.h>

__interface IModule
{
	virtual	void	Destroy()						= NULL;
};

typedef	enum IModuleType {
	IFilterCtrlType,
} IModuleType;

typedef PVOID	(*PCreateInstance)(IN LPVOID p1, IN LPVOID p2);

class CModuleFactory
{
public:
	static	PVOID	CreateInstance(IN PCreateInstance pModule, IN LPVOID p1, IN LPVOID p2)
	{
		if( NULL == pModule )	return NULL;
		return pModule(p1, p2);
	}
	static	void	DestroyInstance(IN IModule * p)
	{
		if( NULL == p )	return;
		p->Destroy();
	}

private:

};