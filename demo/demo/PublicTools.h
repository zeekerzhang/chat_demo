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
//互斥量
#define UNIQUE_APP L"{zeeker_once}"
//窗口句柄
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

	//宽字节转为窄字节(std::wstring  -->  std::string)
	std::string wstring2string(const std::wstring wstr)
	{
		std::string result;
		//获取缓冲区大小，并申请空间，缓冲区大小事按字节计算的
		int len = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), wstr.size(), NULL, 0, NULL, NULL);
		char *buffer = new char[len + 1];
		//宽字节编码转换成多字节编码
		WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), wstr.size(), buffer, len, NULL, NULL);
		buffer[len] = '\0';
		//删除缓冲区并返回值
		result.append(buffer);
		delete[] buffer;
		return result;
	}

	//将string转换成wstring(std::string  -->  std::wstring)
	std::wstring string2wstring(std::string str)
	{
		std::wstring result;
		//获取缓冲区大小，并申请空间，缓冲区大小按字符计算
		int len = MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.size(), NULL, 0);
		wchar_t *buffer = new wchar_t[len + 1];
		//多字节编码转换成宽字节编码
		MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.size(), buffer, len);
		//添加字符串结尾
		buffer[len] = '\0';             
		//删除缓冲区并返回值
		result.append(buffer);
		delete[] buffer;
		return result;
	}
	//获取时间(转lua)
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
	//获取窗口句柄
	HWND GetConsoleHandle()
	{
		HMODULE hKernel32 = GetModuleHandle(L"kernel32");
		ZLGetConsoleWindow = (PROCGETCONSOLEWINDOW)GetProcAddress(hKernel32, "GetConsoleWindow");
		cmd = ZLGetConsoleWindow();
		return cmd;
	}
	//获取CPU核心数
	DWORD GetCPUCount()
	{
		SYSTEM_INFO si;
		GetSystemInfo(&si);
		return si.dwNumberOfProcessors;
	}
	//验证昵称字符串 含有特殊字符
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