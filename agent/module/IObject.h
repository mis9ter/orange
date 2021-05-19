#pragma once

#include <windows.h>
#include <strsafe.h>
#include "yagent.common.h"


interface	IObject
{
	//virtual	void	Retain()		= NULL;
	virtual	void	Release()		= NULL;
	//virtual long	RetainCount()	= 0;
};

class CObject
{
public:
	CObject() {
	}
	~CObject() {
	}
private:

};
