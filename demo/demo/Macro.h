//�궨�壬����ͳһ�޸�

#ifndef MacroH
#define MacroH

//��������Ϣ����
//�յ������
#define WM_ERRORPCK       WM_USER+1001
//����������Ϣ
#define WM_PUBMSG         WM_USER+1002
//�����б�
#define WM_UPDATELIST     WM_USER+1003
//ɾ���б�
#define WM_DELUSERLIST    WM_USER+1004
//������ʾ
#define WM_ONLINEWARN     WM_USER+1005
//��һ�ε�½�ɹ�
#define WM_TRYLOGIN_OK    WM_USER+1006
//��һ�ε�½ʧ��
#define WM_TRYLOGIN_NOK   WM_USER+1007

//��ʾ�ﶨ��
#define STR_LOGIN_SUCCESS(user)     "��ӭ" + user + "��½!\r\n"
#define STR_LOGOUT_SUCCESS			L"���Ѿ��˳�������!"
#define STR_NEED_SENDMSG			L"������Ҫ���͵���Ϣ!"
#define STR_INVALID_NICKNAME		L"��������Ч���ǳ�!"
#define STR_NEED_LOGININFO			L"�������½����Ϣ!"
#define STR_ERROR_PACKET			L"�յ�����İ�"
#define STR_EXIT_CHATROOM			"���˳�������!"
#define STR_ENTER_CHATROOM			" �����ǳ�!"
#define STR_NOLOGIN					L"���ȵ�¼!"
#define STR_SELF_SAY				L"��˵ : "
#define STR_OTHER_SAY				L"˵ : "
#define STR_TOOLONG					L"���͵����ݹ���!"
#define STR_WARNTEXT				L"��ʾ"
#define STR_LOGINFAILED				L"���ӷ����ʧ��!"
#define STR_LOGINING				L"�������ӷ����..."

//���νṹ��
struct LoginParam
{
	std::string   ip;
	int           port;
	HWND          hMain;
};

#endif