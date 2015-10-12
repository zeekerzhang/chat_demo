
// demoDlg.cpp : ʵ���ļ�
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
//��Ϣ����
Queue msg_queue(10240);
//socket���
SocketClient *sClient;
//�����ͻ����б�
typedef std::map<std::string, std::string> ClientMap;
ClientMap cmap;
//����ΨһID
static std::string g_Identify;

CThreadLock g_TGetLock, g_TReadLock;
CThreadLock g_ThreadLock;
//˫�������
PacketQueue doublebuffer_queue(1);

HWND hMain = NULL;
#define DATA_BLOCK_LEN 1024 
CMemPool MsgPool(DATA_BLOCK_LEN, 0, 1024); 

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

void DataProcesser(char *data);
//����guid
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
	//��ȡʱ��(תlua)
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
//�˳��߳�
void ExitProc(void *arg);
//��ȡ��Ϣ������
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
		//�ѽ��bug1:����������ڴ�memset��0�����ڴ�ĵ�ǰֵ�ͻḲ���ϴε�(memcpy)��
		//���ܵ��»���"����β��"�������ϴ�����Ϊ:12345����ε�����������:abc��
		//������ڴ��ֵ���Ϊ:abc45����ע�⣬������ڴ�ر�����ʵ���йء�
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
//��ȡ��Ϣ������Ϣ
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
		//����	
		LOAD_JSONCODER_STR(document, msg);
		JsonCoder jc_decoder;
		if(!jc_decoder.CheckPacket(document))
		{
			//cout << "error packet" << endl;
			PostMessage(hMain, WM_ERRORPCK, 0, 0);
			continue;
		}
		//��ȡ��Ϣ
		uint32 cmd = jc_decoder.GetInt(JC_KEY::JC_MSGTYPE, document);
		//����˷��͹����Ļ�Ӧ��
		if(cmd == MESSAGE_TYPE::cmdLoginRep)
		{
			//���صĿͻ��б�
			std::string     user_list   =  jc_decoder.GetString(JC_KEY::JC_USERLISTREP, document);
			//������ʾ
			std::string		onlineUser  =  jc_decoder.GetString(JC_KEY::JC_ONLINEREP, document);
			//��������������Ϣ
			std::string *olUser = new std::string(onlineUser);
			PostMessage(hMain, WM_ONLINEWARN, (WPARAM)olUser, 0);
			
#ifdef DEBUG_MODE
			cout << "user_list" << user_list << endl;
#endif			
			//�����б���Ϣ
			std::string *uList = new std::string(user_list);
			//���ߵ��û���ʾ
			std::string *warn = new std::string(onlineUser);
			PostMessage(hMain, WM_UPDATELIST, (WPARAM)uList, (LPARAM)warn);
		}
		else if(cmd == MESSAGE_TYPE::cmdLogoutRep)
		{
			//�ͻ���������ʾ
			std::string		logoutUser  =  jc_decoder.GetString(JC_KEY::JC_LOGOUTUSER, document);
			//�б�ɾ��
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
			//������id
			std::string		user_identify  =  jc_decoder.GetString(JC_KEY::JC_IDENTIFY, document);
			//˭���͵�
			std::string		sender  =  jc_decoder.GetString(JC_KEY::JC_SENDER, document);
			//������ʲô
			std::string		pubmsg  =  jc_decoder.GetString(JC_KEY::JC_PUBMSG, document);
			//����ʱ��
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
		//�ڴ�黹���ڴ��
		MsgPool.Release(msg);
	}
}

//ͬ��ģʽ���ӵ������
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

//��½
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
//�ǳ�
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

//������Ϣ
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

// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// �Ի�������
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

// ʵ��
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

//window�Դ��̳߳�(���ڽ�����Ϣ)
DWORD WINAPI RecvMsgThreadPool(PVOID pContext)
{
	CAutoLock lock(&g_ThreadLock);
	char *data = (char *)pContext;
	rcv_queue.Push(data);
	return 1;
}
// CdemoDlg �Ի���




CdemoDlg::CdemoDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CdemoDlg::IDD, pParent)
	, Statu_Login(false)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	//����ID
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
				//��ȡ�ǳ�����
				TCHAR NickName[MAX_PATH] = {0};
				edtNickName.GetWindowTextW(NickName, MAX_PATH);
				std::string nickname = PublicTool::wstring2string(NickName);
				//��ȡ���͵�����
				TCHAR SendText[MAX_PATH] = {0};
				edtSendMsg.GetWindowTextW(SendText, MAX_PATH);
				std::string sendtext = PublicTool::wstring2string(SendText);
				Send(nickname, "�Զ�������Ϣ����");
			}
			break;
		case 2:
			{
				{
					//���ɼ�
					if(!(GetWindowLong(m_hWnd, GWL_STYLE) & WS_VISIBLE))
					{
						//ShowWindow(SW_MINIMIZE);
						//CWnd* pParentWnd = GetTopLevelParent();
						PublicTool::FlashWindow(m_hWnd);
					}
					else
					{
						//�������ϲ�
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
		//::MessageBoxW(this->GetSafeHwnd(), L"test!", L"��ʾ", MB_OK);
		/*����Ѿ���װΪFunAddMsg
		int lastLine = edtShowMsg.LineIndex(edtShowMsg.GetLineCount() - 1);
		edtShowMsg.SetSel(lastLine + 1, lastLine + 2, 0);   
		edtShowMsg.ReplaceSel(L"�յ�����İ�\r\n");  // �����һ������µ�����
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
		//����б�����
		((CListBox*)GetDlgItem(IDC_LIST1))->ResetContent(); 
		std::string *ss = (std::string *)wParam;
		std::string *warn_user = (std::string *)lParam;
		//�ֽ����ӵ��б�
		{
			char *pList = NULL;
			pList = strtok(const_cast<char *>((*ss).c_str()), "|");
			//���
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
		//������ߵ��û������Լ�����ʾ
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
		//���ҳ�index
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
			//�ȷ������߱���
			TCHAR NickName[MAX_PATH] = {0};
			edtNickName.GetWindowTextW(NickName, MAX_PATH);
			std::string nickname = PublicTool::wstring2string(NickName);
			std::string *param_str = new std::string(nickname);
			HANDLE handle = (HANDLE)_beginthread(ExitProc, 0, param_str);
			//�ȴ��߳����,2��󷵻أ�����ִ���������
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
			//��½�����
			edtNickName.EnableWindow(false);
			edtServerIp.EnableWindow(false);
			edtServerPort.EnableWindow(false);
			btnLogin.EnableWindow(false);
			//�����߳�
			try  
			{
				_beginthread(read_func, 0, this->GetSafeHwnd());
				DWORD cpu_count = PublicTool::GetCPUCount();
				//�����߳�
				PoolExecutor CustomerExecutor(1);
				for(int m = 0; m < cpu_count * 2 + 1; m++)
					_beginthread(proc, 0, this->GetSafeHwnd());
			}
			catch(Synchronization_Exception &e)  
			{  
				cerr << e.what() << endl; 
				return -2;
			}
			//��½��û��ʵ�ֵ�½�Ƿ�ɹ���
			TCHAR NickName[20] = {0};
			edtNickName.GetWindowTextW(NickName, 20);

			Login(PublicTool::wstring2string(NickName));
			edtShowMsg.SetWindowTextW(PublicTool::string2wstring(STR_LOGIN_SUCCESS(PublicTool::wstring2string(NickName))).c_str());
			Statu_Login = true;
			//�Զ����ͣ����ԣ�
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

// CdemoDlg ��Ϣ�������

BOOL CdemoDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	//����Ĭ��ip
	edtServerIp.SetWindowTextW(L"183.13.36.10");
	//����Ĭ�϶˿�
	edtServerPort.SetWindowTextW(L"5551");

	// ��������...���˵�����ӵ�ϵͳ�˵��С�

	// IDM_ABOUTBOX ������ϵͳ���Χ�ڡ�
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

	// ���ô˶Ի����ͼ�ꡣ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO: �ڴ���Ӷ���ĳ�ʼ������

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
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

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CdemoDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR CdemoDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

BOOL CdemoDlg::PreTranslateMessage(MSG* pMsg) 
{
	// TODO: Add your specialized code here and/or call the base class
	//�س�������Ϣ
	if(pMsg->message == WM_KEYDOWN) 
	{
		if(VK_RETURN == pMsg->wParam)
		{
			//�����Ƿ���IDC_BUTTON4���Ͱ�ť��
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
		//��������
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

//��½�߳�
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
	// TODO: �ڴ���ӿؼ�֪ͨ����������
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
	//�ж��Ƿ��������ַ�
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
	
	//�״γ������ӷ�����
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
			//��½�����
			edtNickName.EnableWindow(false);
			edtServerIp.EnableWindow(false);
			edtServerPort.EnableWindow(false);
			btnLogin.EnableWindow(false);
			//�����߳�
			//for(int m = 0; m < 1; ++m)
			{
				_beginthread(read_func, 0, this->GetSafeHwnd());
				DWORD cpu_count = PublicTool::GetCPUCount();
				//�����߳�
				PoolExecutor CustomerExecutor(1);
				for(int m = 0; m < cpu_count * 2 + 1; m++)
				{
					_beginthread(proc, 0, this->GetSafeHwnd());
				}
			}
			//��½��û��ʵ�ֵ�½�Ƿ�ɹ���
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
	// TODO: �ڴ���ӿؼ�֪ͨ����������
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
		//ɾ����list
		lbOnline.ResetContent();

		//ע����ָ�
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
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	if(Statu_Login == true)
	{
		//��ȡ�ǳ�����
		TCHAR NickName[MAX_PATH] = {0};
		edtNickName.GetWindowTextW(NickName, MAX_PATH);
		std::string nickname = PublicTool::wstring2string(NickName);
		//��ȡ���͵�����
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
		//����Լ�˵����Ϣ
		FunAddMsg(STR_SELF_SAY + std::wstring(SendText) + L"[" + PublicTool::string2wstring(std::string(PublicTool::GetSysTime())) + L"]");
		//��շ�������
		edtSendMsg.SetWindowTextW(L"");
	}
	else
	{
		::MessageBoxW(this->GetSafeHwnd(), STR_NOLOGIN, STR_WARNTEXT, MB_OK);
	}
}
//�˳��߳�
void ExitProc(void *arg)
{
	std::string *nickname = (std::string *)arg;
	do
	{
		Logout(*nickname);	
	}while(0);
	sClient->Close();
	//::HP_Client_Stop(m_spClient);
	// ���� Socket ����
	//::Destroy_HP_TcpPullClient(m_spClient);
	// ���ټ���������
	//::Destroy_HP_TcpPullClientListener(m_spListener);
	delete nickname;
	return;
}
void CdemoDlg::OnBnClickedButton3()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	if(Statu_Login)
	{
		//�ȷ������߱���
		TCHAR NickName[MAX_PATH] = {0};
		edtNickName.GetWindowTextW(NickName, MAX_PATH);
		std::string nickname = PublicTool::wstring2string(NickName);
		std::string *param_str = new std::string(nickname);
		HANDLE handle = (HANDLE)_beginthread(ExitProc, 0, param_str);
		//�ȴ��߳����,2��󷵻أ�����ִ���������
		::WaitForSingleObject(handle, 2000);
		EndDialog(1);
	}
	else
	{
		EndDialog(1);
	}
}
