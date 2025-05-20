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
       
       //  JSON 문자열을 파싱하여 구조체 멤버 변수 초기화
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
       long long mTimestamp; // SQL의 datetime형식 ->  TIMESTAMP_STRUCT 구조체로 변환해서 사용-> json형태로 저장하기 위해 ISO 8601 형식 문자열로 변환 후 저장
  
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