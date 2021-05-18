#pragma once

class CPathConvertor
{
public:
	CPathConvertor(void)
	{
		InitPathInformation();
	}
	virtual ~CPathConvertor(void)
	{

	}
	void    InitPathInformation(void)
	{
		m_pathMap.clear();

		static const int MAX_DRV_COUNT = 'Z' - 'A';

		WCHAR tmpSymName[8] = { 0 };
		WCHAR tmpDevName[MAX_PATH] = { 0 };
		for (WCHAR name = 'A'; name <= 'Z'; ++name)
		{
			StringCbPrintf(tmpSymName, sizeof(tmpSymName), L"%c:", name);
			QueryDosDevice(tmpSymName, tmpDevName, MAX_PATH);
			m_pathMap[name] = tmpDevName;
		}
	}
	BOOL    MakeDevicePath(PCWSTR pSymbolicPath, PWSTR pDevicePath, SIZE_T devicePathSize)
	{
		WCHAR drive = pSymbolicPath[0];
		PATH_MAP::iterator it = m_pathMap.find(drive);
		if (it == m_pathMap.end())
		{
			InitPathInformation();

			drive = pSymbolicPath[0];
			it = m_pathMap.find(drive);
			if (it == m_pathMap.end())
			{
				return FALSE;
			}
		}
		CString devicePath;
		StringCbPrintf(pDevicePath, devicePathSize, L"%s%s", (it->second), &pSymbolicPath[2]);
		return TRUE;
	}
	CString MakeDevicePath(CString symbolicPath)
	{
		if (PathIsRelative(symbolicPath) || !PathFileExists(symbolicPath))
			return CString(symbolicPath);

		WCHAR drive = symbolicPath.GetAt(0);
		PATH_MAP::iterator it = m_pathMap.find(drive);
		if (it == m_pathMap.end())
		{
			InitPathInformation();

			drive = symbolicPath.GetAt(0);
			it = m_pathMap.find(drive);
			if (it == m_pathMap.end())
			{
				return CString(symbolicPath);
			}
		}
		CString devicePath;
		devicePath.Format(L"%s%s", it->second, symbolicPath.Mid(2));
		return devicePath;
	}
	CString	MakeRegistryPath(CString path)
	{
		const WCHAR* pMachineKey = L"HKEY_LOCAL_MACHINE";
		const WCHAR* pUserKey = L"HKEY_USERS";

		CString			regPath(path);
		if (0 == _wcsnicmp(path, pMachineKey, wcslen(pMachineKey)))
		{
			regPath.Replace(pMachineKey, L"\\REGISTRY\\MACHINE");
		}
		else if (0 == _wcsnicmp(path, pUserKey, wcslen(pUserKey)))
		{
			regPath.Replace(pUserKey, L"\\REGISTRY\\USER");
		}
		return regPath;
	}
	CString MakeSymbolicPath(CString devicePath)
	{
		PATH_MAP::iterator it = m_pathMap.begin();
		for (; it != m_pathMap.end(); ++it)
		{
			CString& travDevPath = it->second;
			SIZE_T length = travDevPath.GetLength();
			if (length > 0)
			{
				if (it->second.CompareNoCase(devicePath.Left((int)length)) == 0)
				{
					CString symbolicPath;
					symbolicPath.Format(L"%c:%s", 
						it->first, (PCWSTR)devicePath.Mid(it->second.GetLength()));
					return symbolicPath;
				}
			}
		}
		return CString(L"");
	}

private:
	typedef std::map<WCHAR, CString> PATH_MAP;
	PATH_MAP m_pathMap;
};