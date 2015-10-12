#ifndef JsonCoderH
#define JsonCoderH

//json
#include "rapidjson/document.h"		// rapidjson's DOM-style API
#include "rapidjson/prettywriter.h"	// for stringify JSON
#include "rapidjson/filestream.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;

namespace JC_KEY
{
	char *   JC_MSGTYPE       =       "MSGTYPE";     //消息类型
	char *   JC_USERCID       =       "USERCID";     //客户端链接id
	char *   JC_IDENTIFY      =       "IDENTIFY";    //唯一ID
	char *   JC_LOGINUSER     =       "LOGINUSER";   //登陆昵称
	char *   JC_LOGINTIME     =       "LOGINTIME";   //登陆时间
	char *   JC_LOGOUTUSER    =       "LOGOUTUSER ";   //登出昵称
	
	char *   JC_SENDER        =       "SENDER";      //发送者
	char *   JC_PUBMSG        =       "PUBMSG";      //发送的消息
	char *   JC_PUBTIME       =       "PUBTIME";     //发送时间
	char *   JC_LOGINREP      =       "LOGINREP";     //登陆回应
	char *   JC_ONLINEREP     =       "ONLINEREP";     //上线提醒
	char *   JC_USERLISTREP   =       "USERLISTREP";     //用户列表

	char *   JC_LOGOUTREP     =       "LOGOUTREP";     //登出回应
	char *   JC_OFFLINEREP    =       "OFFLINEREP";     //离线提醒
	char *   JC_LOGOUTTIME    =       "LOGOUTTIME";   //登出时间

	char *   JC_PUBMSGREP     =       "PUBMSGREP";   //公共聊天回应

	char *   JC_RECIVEMSG     =       "RECIVEMSG ";     //收到来自客户端的信息
}

class JsonCoder
{
public:
	JsonCoder(){}
	JsonCoder(rapidjson::Document &document)
	{
		document.SetObject();
		rapidjson::Document::AllocatorType& allocator = document.GetAllocator();
	}
	~JsonCoder(){}
	//获取int类型的值
	int GetInt(char *tag, rapidjson::Document &document)
	{
		int ret = -1;
		if(document.HasMember(tag))
		{
			if(document[tag].IsInt())
			{
				ret = document[tag].GetInt();
			}
		}	

		return ret;
	}
	//获取string类型的值
	std::string GetString(char *tag, rapidjson::Document &document)
	{
		std::string ret("NULL");
		if(document.HasMember(tag))
		{
			if(document[tag].IsString())
			{
				ret = document[tag].GetString();
			}
		}	
		return ret;
	}
	//
	void SetString(char *key, char *value, rapidjson::Document &document)
	{
		//document.SetObject();
		//rapidjson::Document::AllocatorType& allocator = document.GetAllocator();
		document.AddMember(key, value, document.GetAllocator());
		//rapidjson::StringBuffer buffer;
		//rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
		//document.Accept(writer);
		//auto out = buffer.GetString();
		//return out;
	}
	void SetInteger(char *key, int value, rapidjson::Document &document)
	{

		document.AddMember(key, value, document.GetAllocator());
	}
	std::string GetJson(rapidjson::Document &document)
	{
		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
		document.Accept(writer);
		auto out = buffer.GetString();
		return out;
	}
	bool CheckPacket(rapidjson::Document &document)
	{
		if(document.HasParseError())
		{
			//错误包(格式解析错误)
			//printf("GetParseError %s\n", document.GetParseError());
			return false;
		}
		return true;
	}
private:
	std::string json;
};

#define USE_JSONCODER_DOC(doc) \
	rapidjson::Document doc;
#define LOAD_JSONCODER_STR(doc, str) \
	doc.Parse<0>(str);

#endif