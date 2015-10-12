//---------------------------------------------------------------------------

#ifndef MsgPackCoderH
#define MsgPackCoderH

#include <iostream>
#include "BaseMessageType.h"
//–Ú¡–ªØ
#include "msgpack\msgpack.hpp"
#pragma comment(lib, "msgpack/msgpack.lib")
//---------------------------------------------------------------------------
template<class T>
class MsgPackCoder
{
public:
	MsgPackCoder(){}
	~MsgPackCoder(){}
	
	BYTE *Encoder(T &cmd_t)
	{
		static msgpack::sbuffer buffer;
		msgpack::pack(buffer, cmd_t);
		return (BYTE *)buffer.data();
	}

	bool Decoder(BYTE *buffer, T &cmd_t)
	{
		try
		{
			msgpack::unpacked msg;
			msgpack::unpack(&msg, (char *)buffer, strlen((char *)buffer));
 
			msgpack::object obj = msg.get();

			obj.convert(&cmd_t);
			return true;
		}
		catch(...)
		{
			return false;
		}
	}
};

# endif