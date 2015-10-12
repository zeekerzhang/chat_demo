//---------------------------------------------------------------------------

#ifndef BaseMessageTypeH
#define BaseMessageTypeH

#include "TypeDef.h"
//---------------------------------------------------------------------------
namespace MESSAGE_TYPE
{
	uint32    cmdLogin           =        0x0001;    //登陆
	uint32    cmdLogout          =        0x0002;    //注销
	uint32    cmdPublicMsg       =        0x0003;    //发送公共消息

	uint32    cmdLoginRep        =        0x0041;    //登陆回应
	uint32    cmdLogoutRep       =        0x0042;    //注销回应
	uint32    cmdPublicMsgRep    =        0x0043;    //发送公共消息回应
}

# endif