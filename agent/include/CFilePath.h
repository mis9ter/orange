#pragma once

class CFilePath
	:
	virtual	public	CAppLog
{
public:
	CFilePath() {

	}
	~CFilePath() {

	}
	bool	SetResourceToFile(UINT nId, LPCWSTR pFilePath)
	{
		Log("%s %ws", __FUNCTION__, pFilePath);
		CMemoryPtr	res = GetResourceData(NULL, nId);
		if (res && res->Data() && res->Size())
			if (SetFileData(pFilePath, res->Data(), res->Size(),
				[&](CErrorMessage& err) {

				Log("%-32s (%d)%s", "CFilePath::SetResourceToFile", (DWORD)err, (PCSTR)err);

			})) {
				return true;
			}
			else {
				Log("%s %ws - SetFileData() failed.", __FUNCTION__, pFilePath);
			}
		return false;
	}
	static	LPCTSTR		CheckBadBeomStyle(IN LPCTSTR pStr)
	{
		//	��� ��θ� ǥ���ϱ� ���� ������ �ַ� �ϴ� ���� 
		//	���� ����� ������ config.exe �� ���� �ʰ� \config.exe�� ���°� ����. 
		//	config\config2.cfg �� ���� \config\config2.cfg ��� ���� ��. 
		//	�̷��� �տ� \\�� �پ� �ִ� ��δ� �и� ������̰� �� �˾� �԰ڴµ�
		//	����API���� �˾����� ������ �̷� ��δ� �˾Ƹ԰� ���� \���ش�.

		//	�� ���ڿ��� �ּ� 2�� �̻��� ���� ��������. 
		if (NULL == pStr || NULL == *pStr || NULL == *(pStr + 1))	return pStr;
		//	�ι�° �͵� \�̶�� '\\'�� ���� UNC ��θ� ǥ���� ���̹Ƿ� �ǵ��� ���ƿ�.
		if (_T('\\') == *pStr && _T('\\') != *(pStr + 1))	return pStr + 1;
		return pStr;
	}
	static	bool		ConvertPath(IN LPCTSTR pSrcPath,
		OUT LPTSTR pDestPath, IN DWORD dwDestSize)
	{
		/*
			��� ��� => ���� ���

			[TODO]
			�׿� ȯ�溯���� ������ ��� => ���� ���
		*/
		if (NULL == pSrcPath)	return false;

		pSrcPath = CheckBadBeomStyle(pSrcPath);
		if (::PathIsRelative(pSrcPath))
		{
			//	���⼭ �������� ��츦 �� ó�����ְڴٰ� 
			//	GetFullPathName / _wfullpath �Լ��� ����ߴµ� �ͼ��� �߻�.  => ���� �ľ� �Ұ�. 
			//	MSDN�� ���� ������ ���� ��� ����.
			/*
				https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getfullpathnamea
				Multithreaded applications and shared library code should not use the GetFullPathName function and
				should avoid using relative path names.
				The current directory state written by the SetCurrentDirectory function is stored as a global variable in each process,
				therefore multithreaded applications cannot reliably use this value without possible data corruption
				from other threads that may also be reading or setting this value.

				This limitation also applies to the SetCurrentDirectory and GetCurrentDirectory functions.
				The exception being when the application is guaranteed to be running in a single thread,
				for example parsing file names from the command line argument string in the main thread
				prior to creating any additional threads.

				Using relative path names in multithreaded applications or shared library code can yield unpredictable results and
				is not supported.
			*/
			//	����ζ�� ���� ��η� �����Ѵ�.
			TCHAR	szModulePath[AGENT_PATH_SIZE] = _T("");
			::GetModuleFileName(NULL, szModulePath, sizeof(szModulePath));
			/*
				Path �ø���� ��������, ��� MAX_PATH ��� ���� ������ �־� ���� ������..
			*/
			::PathRemoveFileSpec(szModulePath);
			::PathCombine(pDestPath, szModulePath, pSrcPath);
			return true;
		}
		::StringCbCopy(pDestPath, dwDestSize, pSrcPath);
		return true;
	}
	static	LPCTSTR		GetRelativePath(IN LPCTSTR pSrcPath)
	{
		//	���� ����� �����͸� �޾Ƽ� ���� ��� ��� ��� ����� �������� ��ȯ. 
		//	C:\\My Folder\\Iam.exe\\config\\config2.cfg => config\\config2.cfg
		pSrcPath = CheckBadBeomStyle(pSrcPath);
		if (::PathIsRelative(pSrcPath))
		{
			//	�̹� ���� ���� ��� ��ζ�� �״�� ����
			return pSrcPath;
		}
		TCHAR		szCurPath[AGENT_PATH_SIZE];
		GetModuleFileName(NULL, szCurPath, sizeof(szCurPath));
		PathRemoveFileSpec(szCurPath);

		DWORD		dwSrcLen = lstrlen(pSrcPath);
		DWORD		dwCurLen = lstrlen(szCurPath);

		//	+1 �� ������, ���� ��ΰ� ���� �� �����κ��� ª�ٸ� ������ΰ�? 
		//	�𸣰����� ������ �״�� ����. �߸԰� �� ��ƶ�.
		return (dwSrcLen > dwCurLen) ? (pSrcPath + dwCurLen + 1) : pSrcPath;
	}
	static	CMemoryPtr	GetResourceData(IN HMODULE hModule, IN UINT nResourceId)
	{
		HRSRC		hRes = NULL;
		HGLOBAL		hData = NULL;
		CMemoryPtr	ptr;

		auto		GetModulePath = [&](IN HMODULE hModule, OUT LPTSTR pPath, IN DWORD dwSize)
		{
			GetModuleFileName(hModule, pPath, dwSize);
			return pPath;
		};

		TCHAR		szPath[AGENT_PATH_SIZE] = _T("");
		do
		{
			if (NULL == (hRes = ::FindResource(hModule, MAKEINTRESOURCE(nResourceId), RT_RCDATA)))
			{
				break;
			}
			if (NULL == (hData = ::LoadResource(hModule, hRes)))
			{
				break;
			}
			DWORD	dwSize = ::SizeofResource(hModule, hRes);
			LPVOID	pData = ::LockResource(hData);
			if (pData && dwSize)
			{
				if (ptr = CMemoryFactory::Instance()->AllocatePtr(__FUNCTION__, pData, dwSize))
				{

				}
				else
				{

				}
			}
			else
			{
#ifdef __log_config
				_nlogA(__log_config, "LockResource() failed. path=%s, code=%d",
					GetModulePath(hModule, szPath, sizeof(szPath)), ::GetLastError());
#endif
			}
		} while (false);
		/*
			hData, pData �̷� �͵��� �������� �ʴ� ����:
			�� ���μ����� �����̰� �̹� �޸𸮿� �ö� �־��.
			���μ��� ����� �˾Ƽ� ������ϴ�.
			����������.
		*/
		return ptr;
	}
	static	bool		SetFileData(IN LPCTSTR lpPath, IN LPCVOID pData,
		IN DWORD dwSize,
		IN std::function<void(CErrorMessage& err)> pErrorCallback
	)
	{
		bool	bRet = false;
		DWORD	dwWrittenBytes;

		//	CREATE_ALWAYS
		//	���� ������ �����ϸ鼭 ���� ������ �纸�� �� ũ�� �����ϴ� ��� 
		//	���� ������ ������ �ڿ� ������ ������ �ٱ� �����̴�.
		//	FILE_SHARE_READ
		//	SetFileContent �۾��� ������ ���� ���¿��� �� �ٽ� ȣ��Ǿ� ���׹����� ���� ����
		//	Read�� ����.
		TCHAR	szPath[AGENT_PATH_SIZE] = _T("");
		HANDLE	hFile = INVALID_HANDLE_VALUE;

		ConvertPath(lpPath, szPath, sizeof(szPath));
		::SetFileAttributes(szPath, FILE_ATTRIBUTE_NORMAL);

		hFile = ::CreateFile(
			szPath,
			GENERIC_WRITE,
			FILE_SHARE_READ,
			NULL,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_HIDDEN,
			NULL);
		if (INVALID_HANDLE_VALUE == hFile)
		{
			if (pErrorCallback) {
				CErrorMessage	err(GetLastError());
				pErrorCallback(err);
			}
		}
		else {
			if (::WriteFile(hFile, pData, dwSize, &dwWrittenBytes, NULL) && dwWrittenBytes == dwSize)	bRet = true;
			::CloseHandle(hFile);
		}
		::SetFileAttributes(szPath, FILE_ATTRIBUTE_NORMAL);
		return bRet;
	}
	static	CMemoryPtr	GetFileData(IN LPCTSTR lpPath)
	{
		HANDLE		hFile = INVALID_HANDLE_VALUE;
		LPVOID		pData = NULL;
		DWORD		dwReadBytes = 0;
		DWORD		dwSize = 0;
		CMemoryPtr	ptr;

		TCHAR	szPath[AGENT_PATH_SIZE] = _T("");
		ConvertPath(lpPath, szPath, sizeof(szPath));
		do
		{
			hFile = ::CreateFile(szPath, GENERIC_READ, FILE_SHARE_READ, NULL,
				OPEN_EXISTING, FILE_ATTRIBUTE_ARCHIVE, NULL);
			if (INVALID_HANDLE_VALUE == hFile)
			{
				printf("CreateFile() failed. code=%d\n", ::GetLastError());
				dwSize = 0;
				break;
			}
			dwSize = ::GetFileSize(hFile, NULL);
			if (0 == dwSize)		break;
			ptr = CMemoryFactory::Instance()->AllocatePtr(__FUNCTION__, NULL, dwSize);
			if (nullptr == ptr)		break;
			if ((FALSE == ::ReadFile(hFile, ptr->Data(), ptr->Size(), &dwReadBytes, NULL)) || (dwReadBytes != ptr->Size()))
			{
				ptr = nullptr;
				break;
			}
		} while (false);
		if (INVALID_HANDLE_VALUE != hFile)
			::CloseHandle(hFile);
		return ptr;
	}
private:

};
