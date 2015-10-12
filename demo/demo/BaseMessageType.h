//---------------------------------------------------------------------------

#ifndef BaseMessageTypeH
#define BaseMessageTypeH

#include "TypeDef.h"
//---------------------------------------------------------------------------
namespace MESSAGE_TYPE
{
	uint32    cmdLogin           =        0x0001;    //��½
	uint32    cmdLogout          =        0x0002;    //ע��
	uint32    cmdPublicMsg       =        0x0003;    //���͹�����Ϣ

	uint32    cmdLoginRep        =        0x0041;    //��½��Ӧ
	uint32    cmdLogoutRep       =        0x0042;    //ע����Ӧ
	uint32    cmdPublicMsgRep    =        0x0043;    //���͹�����Ϣ��Ӧ
}

# endif