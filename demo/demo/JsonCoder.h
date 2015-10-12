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
	char *   JC_MSGTYPE       =       "MSGTYPE";     //��Ϣ����
	char *   JC_USERCID       =       "USERCID";     //�ͻ�������id
	char *   JC_IDENTIFY      =       "IDENTIFY";    //ΨһID
	char *   JC_LOGINUSER     =       "LOGINUSER";   //��½�ǳ�
	char *   JC_LOGINTIME     =       "LOGINTIME";   //��½ʱ��
	char *   JC_LOGOUTUSER    =       "LOGOUTUSER ";   //�ǳ��ǳ�
	
	char *   JC_SENDER        =       "SENDER";      //������
	char *   JC_PUBMSG        =       "PUBMSG";      //���͵���Ϣ
	char *   JC_PUBTIME       =       "PUBTIME";     //����ʱ��
	char *   JC_LOGINREP      =       "LOGINREP";     //��½��Ӧ
	char *   JC_ONLINEREP     =       "ONLINEREP";     //��������
	char *   JC_USERLISTREP   =       "USERLISTREP";     //�û��б�

	char *   JC_LOGOUTREP     =       "LOGOUTREP";     //�ǳ���Ӧ
	char *   JC_OFFLINEREP    =       "OFFLINEREP";     //��������
	char *   JC_LOGOUTTIME    =       "LOGOUTTIME";   //�ǳ�ʱ��

	char *   JC_PUBMSGREP     =       "PUBMSGREP";   //���������Ӧ

	char *   JC_RECIVEMSG     =       "RECIVEMSG ";     //�յ����Կͻ��˵���Ϣ
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
	//��ȡint���͵�ֵ
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
	//��ȡstring���͵�ֵ
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
			//�����(��ʽ��������)
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