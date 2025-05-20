#pragma once  
#include "Utility.h"  

namespace Business  
{  
    enum class TableType 
    { 
        Messages,
        Users,
        Unknown 
    };

   struct Data_User
   {  
       std::string mId;  // varchar(16)  
       std::string mPassword;  // varchar(50)  
       long long mCreated_at; // datetime  
       
       //  JSON ���ڿ��� �Ľ��Ͽ� ����ü ��� ���� �ʱ�ȭ
       Data_User(const nlohmann::json& jsonStr)
       {
           mId = jsonStr["id"];
           mPassword = jsonStr["password"];
           mCreated_at = jsonStr["created_at"];
       }

       nlohmann::json static toJson(std::string id, std::string password, std::time_t created_at)
       {  
           nlohmann::json usersJson =
           {
                {"id", id},
                {"password", password},
                {"created_at", created_at}
           };

           return usersJson;
       }  
   };  

   struct Data_Message  
   {  
       int mId;  
       std::string mSender_id; // varchar(16)  
       std::string mReceiver_id;  // varchar(16)  
       std::string mMessage;  // text
       long long mTimestamp; // SQL�� datetime���� ->  TIMESTAMP_STRUCT ����ü�� ��ȯ�ؼ� ���-> json���·� �����ϱ� ���� ISO 8601 ���� ���ڿ��� ��ȯ �� ����
  
       Data_Message(const nlohmann::json jsonStr)
       {
           mId = jsonStr["id"];
           mSender_id = jsonStr["sender_id"];
           mReceiver_id = jsonStr["receiver_id"];

           std::string utf8_message = jsonStr["message"];
           mMessage = Business::Utility::ConvertUTF8toEUC_KR(utf8_message);

           mTimestamp = jsonStr["timestamp"];
       }

       nlohmann::json static toJson(int id, std::string sender_id, std::string receiver_id, std::string message, std::time_t timestamp)
       {
		   nlohmann::json messagesJson =
		   {
			   {"id", id},
			   {"sender_id", sender_id},
			   {"receiver_id", receiver_id},
			   {"message", message},
			   {"timestamp", timestamp}
		   };

		   return messagesJson;
       }
  
   };  
}