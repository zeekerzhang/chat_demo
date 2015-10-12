
// demoDlg.h : 头文件
//

#pragma once
#include "afxwin.h"

#include <string>
#include "Macro.h"
//socket通讯
#include "SocketUnit\helper.h"
#include "SocketUnit\HPSocket4C.h"
//socket通讯库文件
#pragma comment(lib, "SocketUnit/HPSocket4C_U.lib")

// CdemoDlg 对话框
class CdemoDlg : public CDialogEx
{
// 构造
public:
	CdemoDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_DEMO_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

private:
	//登陆标识
	bool Statu_Login;
// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CEdit edtNickName;
	CEdit edtServerIp;
	CEdit edtServerPort;
	CButton btnLogin;
	CButton btnLogout;
	CListBox lbOnline;
	CEdit edtShowMsg;
	CEdit edtSendMsg;
	CButton btnClose;
	CButton btnSend;
	afx_msg void OnBnClickedButton1();
	LRESULT DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam) ;
	afx_msg void OnBnClickedButton2();
	void FunAddMsg(std::wstring msg);
	afx_msg void OnBnClickedButton4();
	afx_msg void OnBnClickedButton3();
	BOOL PreTranslateMessage(MSG* pMsg);
	//抖动窗口
	void ShapeWin();
};
