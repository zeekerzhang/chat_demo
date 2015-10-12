//宏定义，方便统一修改

#ifndef MacroH
#define MacroH

//主程序消息定义
//收到错误包
#define WM_ERRORPCK       WM_USER+1001
//公共聊天信息
#define WM_PUBMSG         WM_USER+1002
//更新列表
#define WM_UPDATELIST     WM_USER+1003
//删除列表
#define WM_DELUSERLIST    WM_USER+1004
//上线提示
#define WM_ONLINEWARN     WM_USER+1005
//第一次登陆成功
#define WM_TRYLOGIN_OK    WM_USER+1006
//第一次登陆失败
#define WM_TRYLOGIN_NOK   WM_USER+1007

//提示语定义
#define STR_LOGIN_SUCCESS(user)     "欢迎" + user + "登陆!\r\n"
#define STR_LOGOUT_SUCCESS			L"你已经退出了聊天!"
#define STR_NEED_SENDMSG			L"请输入要发送的信息!"
#define STR_INVALID_NICKNAME		L"请输入有效的昵称!"
#define STR_NEED_LOGININFO			L"请输入登陆的信息!"
#define STR_ERROR_PACKET			L"收到错误的包"
#define STR_EXIT_CHATROOM			"已退出聊天室!"
#define STR_ENTER_CHATROOM			" 闪亮登场!"
#define STR_NOLOGIN					L"请先登录!"
#define STR_SELF_SAY				L"我说 : "
#define STR_OTHER_SAY				L"说 : "
#define STR_TOOLONG					L"发送的内容过长!"
#define STR_WARNTEXT				L"提示"
#define STR_LOGINFAILED				L"连接服务端失败!"
#define STR_LOGINING				L"正在连接服务端..."

//传参结构体
struct LoginParam
{
	std::string   ip;
	int           port;
	HWND          hMain;
};

#endif