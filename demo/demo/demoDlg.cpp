
// demoDlg.cpp : 实现文件
//
#define WIN32_LEAN_AND_MEAN
#include "stdafx.h"
#include "demo.h"
#include "demoDlg.h"
#include "afxdialogex.h"

#include <objbase.h>
#include <mmsystem.h>  
#include <WINSOCK2.H>
#include <process.h>
#include <map>
#include <string.h>
#include <string>
#include <iostream>
#include "Socket.h"
#include "PublicTools.h"
#include "JsonCoder.h"
#include "MemPool.h"
#include "ThreadProcess.h"
#include "LockFreeQueue.h"
#include "BaseMessageType.h"
#include "CircularQueue.h"


#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib, "WINMM.LIB") 
#pragma warning(disable:4996)

using namespace lock_free;
//消息队列
Queue msg_queue(10240);
//socket组件
SocketClient *sClient;
//其他客户端列表
typedef std::map<std::string, std::string> ClientMap;
ClientMap cmap;
//保存唯一ID
static std::string g_Identify;

CThreadLock g_TGetLock, g_TReadLock;
CThreadLock g_ThreadLock;
//双缓冲队列
PacketQueue doublebuffer_queue(1);

HWND hMain = NULL;
#define DATA_BLOCK_LEN 1024 
CMemPool MsgPool(DATA_BLOCK_LEN, 0, 1024); 

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

void DataProcesser(char *data);
//生成guid
const char* newGUID()
{
    static char buf[64] = {0};
    GUID guid;
    CoInitialize(NULL);
    if (S_OK == ::CoCreateGuid(&guid))
    {
        _snprintf(buf, sizeof(buf),
            "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
            guid.Data1,
            guid.Data2,
            guid.Data3,
            guid.Data4[0], guid.Data4[1],
            guid.Data4[2], guid.Data4[3],
            guid.Data4[4], guid.Data4[5],
            guid.Data4[6], guid.Data4[7]);
    }
    CoUninitialize();
    return (const char*)buf;
}
	//获取时间(转lua)
	char *GetSysTime()
	{
		SYSTEMTIME SystemTime;
		::GetLocalTime(&SystemTime);
		static char buff[20] = {0};
		memset(buff, 0, 20);
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
//退出线程
void ExitProc(void *arg);
//读取信息到队列
void read_func(void *arg)
{ 
	while(1)
    {
        Sleep(1);
		CAutoLock lock(&g_TReadLock);
		std::string rcv("");
		try
		{
			rcv = sClient->ReceiveBytes();
		}
		catch(...)
		{
			//Log("read_func : MsClient->ReceiveBytes() fialed.");
			continue;
		}
        if(strlen(rcv.c_str()) == 0)
        {
            continue;
        }
	#ifdef  DEBUG_MODE
		std::cout << "read_func : " << rcv << std::endl;
	#endif
		char *data_buff = NULL;
		data_buff = reinterpret_cast<char *>(MsgPool.Get());
		//已解决bug1:如果不对其内存memset清0，，内存的当前值就会覆盖上次的(memcpy)，
		//可能导致会有"垃圾尾巴"（例如上次内容为:12345，这次的期望内容是:abc，
		//最后在内存的值会变为:abc45，忘注意，这个和内存池本来的实现有关。
		if(data_buff)
		{
			memset(data_buff, 0, DATA_BLOCK_LEN);
			memcpy(data_buff, rcv.c_str(), strlen(rcv.c_str()));
			msg_queue.Push(data_buff);
			//(&doublebuffer_queue)->Push(data_buff);
		}
		else
		{
			//Log("read_func : MsgPool.Get() fialed.");
		}
    }
}
//获取消息队列消息
void proc(void *arg)
{
    HWND hMain = (HWND)arg;
	USE_JSONCODER_DOC(document);
	while(1)
    {
        Sleep(1);
		CAutoLock lock(&g_TGetLock);
		char *msg = NULL;
		try
		{
			msg = reinterpret_cast<char *>(msg_queue.Pop());
			//msg = (&doublebuffer_queue)->Popup();
			if(!msg)
			{
				continue;
			}
		}
		catch(...){
			//Log("proc : msg_queue.Pop() fialed.");
		}

	#ifdef  DEBUG_MODE
		std::cout << msg << std::endl;
	#endif	
		//解码	
		LOAD_JSONCODER_STR(document, msg);
		JsonCoder jc_decoder;
		if(!jc_decoder.CheckPacket(document))
		{
			//cout << "error packet" << endl;
			PostMessage(hMain, WM_ERRORPCK, 0, 0);
			continue;
		}
		//获取消息
		uint32 cmd = jc_decoder.GetInt(JC_KEY::JC_MSGTYPE, document);
		//服务端发送过来的回应包
		if(cmd == MESSAGE_TYPE::cmdLoginRep)
		{
			//返回的客户列表
			std::string     user_list   =  jc_decoder.GetString(JC_KEY::JC_USERLISTREP, document);
			//上线提示
			std::string		onlineUser  =  jc_decoder.GetString(JC_KEY::JC_ONLINEREP, document);
			//发送上线提醒消息
			std::string *olUser = new std::string(onlineUser);
			PostMessage(hMain, WM_ONLINEWARN, (WPARAM)olUser, 0);
			
#ifdef DEBUG_MODE
			cout << "user_list" << user_list << endl;
#endif			
			//发送列表消息
			std::string *uList = new std::string(user_list);
			//上线的用户提示
			std::string *warn = new std::string(onlineUser);
			PostMessage(hMain, WM_UPDATELIST, (WPARAM)uList, (LPARAM)warn);
		}
		else if(cmd == MESSAGE_TYPE::cmdLogoutRep)
		{
			//客户端离线提示
			std::string		logoutUser  =  jc_decoder.GetString(JC_KEY::JC_LOGOUTUSER, document);
			//列表删除
			ClientMap::iterator ite = cmap.begin();
			ite = cmap.find(logoutUser);
			if(ite != cmap.end())
			{
				cmap.erase(ite);				
			}
			std::string *ss = new std::string(logoutUser);
			PostMessage(hMain, WM_DELUSERLIST, (WPARAM)ss, 0);
		}
		else if(cmd == MESSAGE_TYPE::cmdPublicMsgRep)
		{
			//发送者id
			std::string		user_identify  =  jc_decoder.GetString(JC_KEY::JC_IDENTIFY, document);
			//谁发送的
			std::string		sender  =  jc_decoder.GetString(JC_KEY::JC_SENDER, document);
			//发送了什么
			std::string		pubmsg  =  jc_decoder.GetString(JC_KEY::JC_PUBMSG, document);
			//发送时间
			std::string		pubtime  =  jc_decoder.GetString(JC_KEY::JC_PUBTIME, document);
			if(strcmp(user_identify.c_str(), g_Identify.c_str()) != 0)	
			{
				std::wstring tmp = PublicTool::string2wstring(sender);
				tmp.append(STR_OTHER_SAY);
				tmp.append(PublicTool::string2wstring(pubmsg));
				tmp.append(L"[");
				tmp.append(PublicTool::string2wstring(pubtime));
				tmp.append(L"]");
				std::wstring *ss = new std::wstring(tmp);
				PostMessage(hMain, WM_PUBMSG, (WPARAM)ss, 0);
			}
		}
		//内存归还到内存池
		MsgPool.Release(msg);
	}
}

//同步模式连接到服务端
bool ConnectServer(std::string ip, USHORT port)
{
	bool ret(false);
	try
	{
		sClient = new SocketClient(ip.c_str(), port);
		ret = true;
	}
	catch(...)
	{
		
	}
	return ret;
}

//登陆
void Login(std::string username)
{

	USE_JSONCODER_DOC(document);
	JsonCoder jc_encoder(document);

	jc_encoder.SetInteger(JC_KEY::JC_MSGTYPE, MESSAGE_TYPE::cmdLogin, document);
	jc_encoder.SetString(JC_KEY::JC_IDENTIFY, const_cast<char *>(g_Identify.c_str()), document);
	jc_encoder.SetString(JC_KEY::JC_LOGINUSER, const_cast<char *>(username.c_str()), document);
	jc_encoder.SetString(JC_KEY::JC_LOGINTIME, GetSysTime(), document);
	std::string json_str = jc_encoder.GetJson(document);

	sClient->SendBytes(json_str);
}
//登出
void Logout(std::string username)
{
	USE_JSONCODER_DOC(document);
	JsonCoder jc_encoder(document);

	jc_encoder.SetInteger(JC_KEY::JC_MSGTYPE, MESSAGE_TYPE::cmdLogout, document);
	jc_encoder.SetString(JC_KEY::JC_IDENTIFY, const_cast<char *>(g_Identify.c_str()), document);
	jc_encoder.SetString(JC_KEY::JC_LOGOUTUSER, const_cast<char *>(username.c_str()), document);
	jc_encoder.SetString(JC_KEY::JC_LOGOUTTIME, GetSysTime(), document);
	std::string json_str = jc_encoder.GetJson(document);

	sClient->SendBytes(json_str);	
}

//发送信息
void Send(std::string username, std::string msg)
{
	USE_JSONCODER_DOC(document);
	JsonCoder jc_encoder(document);

	jc_encoder.SetInteger(JC_KEY::JC_MSGTYPE, MESSAGE_TYPE::cmdPublicMsg, document);
	jc_encoder.SetString(JC_KEY::JC_IDENTIFY, const_cast<char *>(g_Identify.c_str()), document);
	jc_encoder.SetString(JC_KEY::JC_SENDER, const_cast<char *>(username.c_str()), document);
	jc_encoder.SetString(JC_KEY::JC_PUBMSG, const_cast<char *>(msg.c_str()), document);
	jc_encoder.SetString(JC_KEY::JC_PUBTIME, GetSysTime(), document);
	std::string json_str = jc_encoder.GetJson(document);

	sClient->SendBytes(json_str);
}

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

//window自带线程池(用于接收消息)
DWORD WINAPI RecvMsgThreadPool(PVOID pContext)
{
	CAutoLock lock(&g_ThreadLock);
	char *data = (char *)pContext;
	rcv_queue.Push(data);
	return 1;
}
// CdemoDlg 对话框




CdemoDlg::CdemoDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CdemoDlg::IDD, pParent)
	, Statu_Login(false)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	//生成ID
	g_Identify = newGUID();

	hMain = this->GetSafeHwnd();
}

void CdemoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT1, edtNickName);
	DDX_Control(pDX, IDC_EDIT2, edtServerIp);
	DDX_Control(pDX, IDC_EDIT3, edtServerPort);
	DDX_Control(pDX, IDC_BUTTON1, btnLogin);
	DDX_Control(pDX, IDC_BUTTON2, btnLogout);
	DDX_Control(pDX, IDC_LIST1, lbOnline);
	DDX_Control(pDX, IDC_EDIT4, edtShowMsg);
	DDX_Control(pDX, IDC_EDIT5, edtSendMsg);
	DDX_Control(pDX, IDC_BUTTON3, btnClose);
	DDX_Control(pDX, IDC_BUTTON4, btnSend);
}

BEGIN_MESSAGE_MAP(CdemoDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_TIMER()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &CdemoDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CdemoDlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON4, &CdemoDlg::OnBnClickedButton4)
	ON_BN_CLICKED(IDC_BUTTON3, &CdemoDlg::OnBnClickedButton3)
END_MESSAGE_MAP()
//timer
void CdemoDlg::OnTimer(UINT_PTR nIDEvent)   
{
	switch (nIDEvent)
	{
		case 1:  
			{
				//获取昵称内容
				TCHAR NickName[MAX_PATH] = {0};
				edtNickName.GetWindowTextW(NickName, MAX_PATH);
				std::string nickname = PublicTool::wstring2string(NickName);
				//获取发送的内容
				TCHAR SendText[MAX_PATH] = {0};
				edtSendMsg.GetWindowTextW(SendText, MAX_PATH);
				std::string sendtext = PublicTool::wstring2string(SendText);
				Send(nickname, "自动发送消息测试");
			}
			break;
		case 2:
			{
				{
					//不可见
					if(!(GetWindowLong(m_hWnd, GWL_STYLE) & WS_VISIBLE))
					{
						//ShowWindow(SW_MINIMIZE);
						//CWnd* pParentWnd = GetTopLevelParent();
						PublicTool::FlashWindow(m_hWnd);
					}
					else
					{
						//不是最上层
						if(GetForegroundWindow() != GetTopLevelParent())
						{
							PublicTool::FlashWindow(m_hWnd);
						}			
					}
				}
		
				if(GetForegroundWindow() == GetTopLevelParent())
				{
					KillTimer(2);
					PublicTool::FlashWindow(m_hWnd, true);
				}
			
			}
			break;
		default:      
			break;
	}
	CDialogEx::OnTimer(nIDEvent);
}

LRESULT CdemoDlg::DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
	// TODO: Add your specialized code here and/or call the base class
	if(message == WM_ERRORPCK)
	{
		//::MessageBoxW(this->GetSafeHwnd(), L"test!", L"提示", MB_OK);
		/*这段已经封装为FunAddMsg
		int lastLine = edtShowMsg.LineIndex(edtShowMsg.GetLineCount() - 1);
		edtShowMsg.SetSel(lastLine + 1, lastLine + 2, 0);   
		edtShowMsg.ReplaceSel(L"收到错误的包\r\n");  // 在最后一行添加新的内容
		*/
		FunAddMsg(STR_ERROR_PACKET);
	}
	else if(message == WM_PUBMSG)
	{
		std::wstring *ss = (std::wstring *)wParam;
		FunAddMsg(*ss);
		delete ss;
	}
	else if(message == WM_UPDATELIST)
	{
		//清空列表内容
		((CListBox*)GetDlgItem(IDC_LIST1))->ResetContent(); 
		std::string *ss = (std::string *)wParam;
		std::string *warn_user = (std::string *)lParam;
		//分解后添加到列表
		{
			char *pList = NULL;
			pList = strtok(const_cast<char *>((*ss).c_str()), "|");
			//添加
			lbOnline.AddString(PublicTool::string2wstring(pList).c_str());
			if(pList)
			{
				while(pList)
				{
					pList = strtok(NULL, "|");
					if(pList)
					{
						lbOnline.AddString(PublicTool::string2wstring(pList).c_str());
					}
				}
			}
		}
		//如果上线的用户不是自己才提示
		TCHAR NickName[MAX_PATH] = {0};
		edtNickName.GetWindowTextW(NickName, MAX_PATH);
		std::string nickname = PublicTool::wstring2string(NickName);
		std::string m_warn_user = *warn_user;
		if(strcmp(m_warn_user.c_str(), nickname.c_str()) != 0)
		{
			FunAddMsg(PublicTool::string2wstring(*warn_user + STR_ENTER_CHATROOM).c_str());
		}
		delete ss;
		delete warn_user;
	}
	else if(WM_DELUSERLIST == message)
	{
		std::string *ss = (std::string *)wParam;
		//查找出index
		int nIndex = lbOnline.FindString(-1, PublicTool::string2wstring(*ss).c_str());
		if(nIndex != -1)
		{
			lbOnline.DeleteString(nIndex);
		}
		FunAddMsg(PublicTool::string2wstring((*ss) + STR_EXIT_CHATROOM).c_str());
		delete ss;
	}
	else if(message == WM_CLOSE)
	{
		if(Statu_Login)
		{
			//先发送离线报文
			TCHAR NickName[MAX_PATH] = {0};
			edtNickName.GetWindowTextW(NickName, MAX_PATH);
			std::string nickname = PublicTool::wstring2string(NickName);
			std::string *param_str = new std::string(nickname);
			HANDLE handle = (HANDLE)_beginthread(ExitProc, 0, param_str);
			//等待线程完成,2秒后返回，继续执行下面语句
			::WaitForSingleObject(handle, 2000);

			EndDialog(1);
		}
	}
	else if(WM_TRYLOGIN_OK == message)
	{
		//std::string *ip = (std::string *)wParam;
		//int *port = (int *)lParam;
		//std::string sip = *ip;
		//int sport = *port;
		//if(ConnectServer(sip.c_str(), sport))
		{
			//登陆后禁用
			edtNickName.EnableWindow(false);
			edtServerIp.EnableWindow(false);
			edtServerPort.EnableWindow(false);
			btnLogin.EnableWindow(false);
			//启动线程
			try  
			{
				_beginthread(read_func, 0, this->GetSafeHwnd());
				DWORD cpu_count = PublicTool::GetCPUCount();
				//启动线程
				PoolExecutor CustomerExecutor(1);
				for(int m = 0; m < cpu_count * 2 + 1; m++)
					_beginthread(proc, 0, this->GetSafeHwnd());
			}
			catch(Synchronization_Exception &e)  
			{  
				cerr << e.what() << endl; 
				return -2;
			}
			//登陆（没有实现登陆是否成功）
			TCHAR NickName[20] = {0};
			edtNickName.GetWindowTextW(NickName, 20);

			Login(PublicTool::wstring2string(NickName));
			edtShowMsg.SetWindowTextW(PublicTool::string2wstring(STR_LOGIN_SUCCESS(PublicTool::wstring2string(NickName))).c_str());
			Statu_Login = true;
			//自动发送（测试）
			//SetTimer(1, 1000, NULL);
		}
	}
	else if(WM_TRYLOGIN_NOK == message)
	{
		FunAddMsg(STR_LOGINFAILED);
		btnLogin.EnableWindow(true);
	}
	return CDialog::DefWindowProc(message, wParam, lParam);
}

void CdemoDlg::FunAddMsg(std::wstring msg)
{
	int lastLine = edtShowMsg.LineIndex(edtShowMsg.GetLineCount() - 1);
	edtShowMsg.SetSel(lastLine + 1, lastLine + 2, 0);   
	edtShowMsg.ReplaceSel(std::wstring(msg + L"\r\n").c_str());

	if(edtShowMsg.GetLineCount() > 50)
	{
		edtShowMsg.SetWindowTextW(L"");
	}
}

// CdemoDlg 消息处理程序

BOOL CdemoDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	//设置默认ip
	edtServerIp.SetWindowTextW(L"183.13.36.10");
	//设置默认端口
	edtServerPort.SetWindowTextW(L"5551");

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CdemoDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CdemoDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CdemoDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

BOOL CdemoDlg::PreTranslateMessage(MSG* pMsg) 
{
	// TODO: Add your specialized code here and/or call the base class
	//回车发送消息
	if(pMsg->message == WM_KEYDOWN) 
	{
		if(VK_RETURN == pMsg->wParam)
		{
			//焦点是否在IDC_BUTTON4发送按钮上
			//if(GetDlgItem(IDC_BUTTON4) != GetFocus())
			//{
				OnBnClickedButton4();
		//	}
			return true;
		}
	}
	return CDialogEx::PreTranslateMessage(pMsg);
}
void CdemoDlg::ShapeWin()
{
		//抖动窗口
		CRect rect;  
		GetWindowRect(&rect);  
		int m_move = 10;  
		//PlaySound("shake.wav",NULL,SND_FILENAME | SND_ASYNC);  
		for(int i = 1; i < 9; i++)  
		{  
			rect.OffsetRect(0, m_move);  
			MoveWindow(&rect);  
			Sleep(50);  
			rect.OffsetRect(m_move, 0);  
			MoveWindow(&rect);  
			Sleep(50);  
			if(10 == m_move)  
			{  
				m_move =- 10;  
			}  
			else  
			{  
				m_move = 10;  
			}  
		}
}

//登陆线程
void LoginThread(void *arg)
{
	LoginParam *lp = (LoginParam *)arg;
	bool TryConn = false;
	try
	{
		sClient = new SocketClient(lp->ip, lp->port);
		TryConn = true;
	}
	catch(...)
	{

	}
	if(TryConn)
	{
		//PostMessage(lp->hMain, WM_TRYLOGIN_OK, (WPARAM)&(lp->ip), (LPARAM)&(lp->port));
		PostMessage(lp->hMain, WM_TRYLOGIN_OK, 0, 0);
	}
	else
	{
		PostMessage(lp->hMain, WM_TRYLOGIN_NOK, 0, 0);
	}
	delete lp;
}
void CdemoDlg::OnBnClickedButton1()
{
	// TODO: 在此添加控件通知处理程序代码
	TCHAR NickName[MAX_PATH] = {0};
	TCHAR ServerIp[20] = {0};
	TCHAR ServerPort[8] = {0};
	edtNickName.GetWindowTextW(NickName, MAX_PATH);
	edtServerIp.GetWindowTextW(ServerIp, 20);
	edtServerPort.GetWindowTextW(ServerPort, 8);

	if(wcslen(NickName) == 0 || wcslen(ServerIp) == 0 || wcslen(ServerPort) == 0)
	{
		::MessageBoxW(this->GetSafeHwnd(), STR_NEED_LOGININFO, STR_WARNTEXT, MB_OK);
		return;
	}
	//判断是否函数特殊字符
	std::string nickname = PublicTool::wstring2string(NickName);
	if(PublicTool::Verify(const_cast<char *>(nickname.c_str())))
	{
		::MessageBoxW(this->GetSafeHwnd(), STR_INVALID_NICKNAME, STR_WARNTEXT, MB_OK);
		return;
	}

	btnLogin.EnableWindow(false);

	FunAddMsg(STR_LOGINING);

	int port = PublicTool::string2int(PublicTool::wstring2string(ServerPort));
	std::string ip = PublicTool::wstring2string(ServerIp);
	/*
	int *pPort = new int(port);
	std::string *pIp = new std::string(ip);

	LoginParam *lp = new LoginParam;
	lp->hMain = this->GetSafeHwnd();
	lp->ip = *pIp;
	lp->port = *pPort;
	_beginthread(LoginThread, 0, lp);
	*/	
	
	//首次尝试连接服务器
	SocketClient *ConnSocket = NULL;
	bool FirstConn = false;
	try
	{
		ConnSocket = new SocketClient(ip, port);
		FirstConn = true;
	}
	catch(...)
	{

	}
	if(FirstConn)
	{
		if(ConnectServer(ip.c_str(), port))
		{
			//登陆后禁用
			edtNickName.EnableWindow(false);
			edtServerIp.EnableWindow(false);
			edtServerPort.EnableWindow(false);
			btnLogin.EnableWindow(false);
			//启动线程
			//for(int m = 0; m < 1; ++m)
			{
				_beginthread(read_func, 0, this->GetSafeHwnd());
				DWORD cpu_count = PublicTool::GetCPUCount();
				//启动线程
				PoolExecutor CustomerExecutor(1);
				for(int m = 0; m < cpu_count * 2 + 1; m++)
				{
					_beginthread(proc, 0, this->GetSafeHwnd());
				}
			}
			//登陆（没有实现登陆是否成功）
			Login(nickname);
		}
		edtShowMsg.SetWindowTextW(PublicTool::string2wstring(STR_LOGIN_SUCCESS(nickname)).c_str());
		Statu_Login = true;
	}
	else
	{
		FunAddMsg(STR_LOGINFAILED);
		btnLogin.EnableWindow(true);
	}
}


void CdemoDlg::OnBnClickedButton2()
{
	// TODO: 在此添加控件通知处理程序代码
	/*
	if(Statu_Login)
	{
		edtShowMsg.SetWindowTextW(L"");
		
		TCHAR NickName[MAX_PATH] = {0};
		edtNickName.GetWindowTextW(NickName, MAX_PATH);
		std::string nickname = PublicTool::wstring2string(NickName);
		Logout(nickname);

		FunAddMsg(STR_LOGOUT_SUCCESS);
		sClient->Close();
		Statu_Login = false;
		//删除掉list
		lbOnline.ResetContent();

		//注销后恢复
		edtNickName.EnableWindow(true);
		edtServerIp.EnableWindow(true);
		edtServerPort.EnableWindow(true);
		btnLogin.EnableWindow(true);
	}
	else
	{
		::MessageBoxW(this->GetSafeHwnd(), STR_NOLOGIN, STR_WARNTEXT, MB_OK);  
	}
	*/
	//SetTimer(2, 1000, NULL);
}

void CdemoDlg::OnBnClickedButton4()
{
	// TODO: 在此添加控件通知处理程序代码
	if(Statu_Login == true)
	{
		//获取昵称内容
		TCHAR NickName[MAX_PATH] = {0};
		edtNickName.GetWindowTextW(NickName, MAX_PATH);
		std::string nickname = PublicTool::wstring2string(NickName);
		//获取发送的内容
		TCHAR SendText[MAX_PATH] = {0};
		edtSendMsg.GetWindowTextW(SendText, MAX_PATH);
		if(wcslen(SendText) == 0)
		{
			::MessageBoxW(this->GetSafeHwnd(), STR_NEED_SENDMSG, STR_WARNTEXT, MB_OK);
			return;
		}
		if(wcslen(SendText) >= 50)
		{
			::MessageBoxW(this->GetSafeHwnd(), STR_TOOLONG, STR_WARNTEXT, MB_OK);
			return;
		}
		std::string sendtext = PublicTool::wstring2string(SendText);
		Send(nickname, sendtext);
		//添加自己说的信息
		FunAddMsg(STR_SELF_SAY + std::wstring(SendText) + L"[" + PublicTool::string2wstring(std::string(PublicTool::GetSysTime())) + L"]");
		//清空发送内容
		edtSendMsg.SetWindowTextW(L"");
	}
	else
	{
		::MessageBoxW(this->GetSafeHwnd(), STR_NOLOGIN, STR_WARNTEXT, MB_OK);
	}
}
//退出线程
void ExitProc(void *arg)
{
	std::string *nickname = (std::string *)arg;
	do
	{
		Logout(*nickname);	
	}while(0);
	sClient->Close();
	//::HP_Client_Stop(m_spClient);
	// 销毁 Socket 对象
	//::Destroy_HP_TcpPullClient(m_spClient);
	// 销毁监听器对象
	//::Destroy_HP_TcpPullClientListener(m_spListener);
	delete nickname;
	return;
}
void CdemoDlg::OnBnClickedButton3()
{
	// TODO: 在此添加控件通知处理程序代码
	if(Statu_Login)
	{
		//先发送离线报文
		TCHAR NickName[MAX_PATH] = {0};
		edtNickName.GetWindowTextW(NickName, MAX_PATH);
		std::string nickname = PublicTool::wstring2string(NickName);
		std::string *param_str = new std::string(nickname);
		HANDLE handle = (HANDLE)_beginthread(ExitProc, 0, param_str);
		//等待线程完成,2秒后返回，继续执行下面语句
		::WaitForSingleObject(handle, 2000);
		EndDialog(1);
	}
	else
	{
		EndDialog(1);
	}
}
