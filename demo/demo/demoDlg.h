
// demoDlg.h : ͷ�ļ�
//

#pragma once
#include "afxwin.h"

#include <string>
#include "Macro.h"
//socketͨѶ
#include "SocketUnit\helper.h"
#include "SocketUnit\HPSocket4C.h"
//socketͨѶ���ļ�
#pragma comment(lib, "SocketUnit/HPSocket4C_U.lib")

// CdemoDlg �Ի���
class CdemoDlg : public CDialogEx
{
// ����
public:
	CdemoDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_DEMO_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��

private:
	//��½��ʶ
	bool Statu_Login;
// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
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
	//��������
	void ShapeWin();
};
