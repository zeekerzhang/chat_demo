//---------------------------------------------------------------------------

#ifndef PublicToolsH
#define PublicToolsH
#include <windows.h>
#include <string>
#include <sstream>
#include "deelx.h"
//warning C4996: 'sprintf': This function or variable may be unsafe. Consider using sprintf_s instead. 
//To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details.
#pragma warning(disable:4996)
//������
#define UNIQUE_APP L"{zeeker_once}"
//���ھ��
HWND cmd;
typedef HWND (WINAPI *PROCGETCONSOLEWINDOW)();
PROCGETCONSOLEWINDOW ZLGetConsoleWindow;
//---------------------------------------------------------------------------
namespace PublicTool
{
	bool RunOnce()
	{
		HANDLE hOneInstance;  
		hOneInstance = ::CreateMutex(NULL, FALSE, UNIQUE_APP);  
		if(GetLastError() == ERROR_ALREADY_EXISTS)  
		{  
			return false;  
		}  
		return true;
	}

	std::string int2string(const int n)
	{
		std::stringstream newstr;
		newstr << n;
		return newstr.str();
	}

	int string2int(std::string str)
	{
		return atoi(str.c_str());		
	}

	//���ֽ�תΪխ�ֽ�(std::wstring  -->  std::string)
	std::string wstring2string(const std::wstring wstr)
	{
		std::string result;
		//��ȡ��������С��������ռ䣬��������С�°��ֽڼ����
		int len = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), wstr.size(), NULL, 0, NULL, NULL);
		char *buffer = new char[len + 1];
		//���ֽڱ���ת���ɶ��ֽڱ���
		WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), wstr.size(), buffer, len, NULL, NULL);
		buffer[len] = '\0';
		//ɾ��������������ֵ
		result.append(buffer);
		delete[] buffer;
		return result;
	}

	//��stringת����wstring(std::string  -->  std::wstring)
	std::wstring string2wstring(std::string str)
	{
		std::wstring result;
		//��ȡ��������С��������ռ䣬��������С���ַ�����
		int len = MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.size(), NULL, 0);
		wchar_t *buffer = new wchar_t[len + 1];
		//���ֽڱ���ת���ɿ��ֽڱ���
		MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.size(), buffer, len);
		//����ַ�����β
		buffer[len] = '\0';             
		//ɾ��������������ֵ
		result.append(buffer);
		delete[] buffer;
		return result;
	}
	//��ȡʱ��(תlua)
	char *GetSysTime()
	{
		SYSTEMTIME SystemTime;
		::GetLocalTime(&SystemTime);
		static char buff[20] = {0};
		sprintf(buff, 
				"%d-%d-%d %02d:%02d:%02d", 
				SystemTime.wYear, 
				SystemTime.wMonth,
				SystemTime.wDay,
				SystemTime.wHour,
				SystemTime.wMinute,
				SystemTime.wSecond);
		return buff;
	}
	//��ȡ���ھ��
	HWND GetConsoleHandle()
	{
		HMODULE hKernel32 = GetModuleHandle(L"kernel32");
		ZLGetConsoleWindow = (PROCGETCONSOLEWINDOW)GetProcAddress(hKernel32, "GetConsoleWindow");
		cmd = ZLGetConsoleWindow();
		return cmd;
	}
	//��ȡCPU������
	DWORD GetCPUCount()
	{
		SYSTEM_INFO si;
		GetSystemInfo(&si);
		return si.dwNumberOfProcessors;
	}
	//��֤�ǳ��ַ��� ���������ַ�
	bool Verify(char *str) 
	{
		bool get = false;
		if(!str)
			return get;
		CRegexpT <char> regexp("((?=[\x21-\x7e]+)[^A-Za-z0-9])");
		MatchResult result = regexp.Match(str);	
		while(result.IsMatched())
		{
			get = true;
			break;
		}
		return get;
	}
	void FlashWindow(HWND hWnd, bool stop = false)
	{
		if(HINSTANCE hUser = LoadLibrary(_T("User32")))
		{
			BOOL (WINAPI *pfnFlashWindowEx)(PFLASHWINFO pfwi);
			(FARPROC&)pfnFlashWindowEx = GetProcAddress(hUser, "FlashWindowEx");
			if(pfnFlashWindowEx)
			{
				FLASHWINFO pFWX;
				pFWX.cbSize	= sizeof(pFWX);
				if(!stop)
				{
					pFWX.dwFlags	= 0x00000003 | 0x0000000C ;//FLASHW_ALL | FLASHW_TIMERNOFG;
				}
				else
				{
					pFWX.dwFlags    = FLASHW_STOP;
				}
				pFWX.uCount	= 3;
				pFWX.dwTimeout	= 0;
				pFWX.hwnd	= hWnd;//pParentWnd->GetSafeHwnd();
				(*pfnFlashWindowEx)(&pFWX);
			}
			FreeLibrary(hUser);
		}
	}
}
#endif