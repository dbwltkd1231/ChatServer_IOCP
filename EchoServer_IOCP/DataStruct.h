#pragma once  
#include <string>  
#include <chrono>  
#include <nlohmann/json.hpp> // Ensure the correct header file is included  

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
       std::string id;  // varchar(16)  
       std::string password;  // varchar(50)  
       long long created_at; // datetime  
       


       //  JSON ���ڿ��� �Ľ��Ͽ� ����ü ��� ���� �ʱ�ȭ
       Data_User(const nlohmann::json& jsonStr)
       {
           id = jsonStr["id"];
           password = jsonStr["password"];
           created_at = jsonStr["created_at"];
       }

       std::string toJson() const {  
           nlohmann::json j; // Ensure the nlohmann namespace is correctly resolved  
           j["id"] = id;  
           j["password"] = password;  
           j["created_at"] = created_at;
           return j.dump();  
       }  

       /*
       INSERT INTO Users (id, password, created_at) VALUES ('user1234', 'securePass123!', '2025-05-16 14:30:00');
       */
   };  

   struct Data_Message  
   {  
       int id;  
       std::string sender_id; // varchar(16)  
       std::string receiver_id;  // varchar(16)  
       std::string message;  // text
       long long timestamp; // SQL�� datetime���� ->  TIMESTAMP_STRUCT ����ü�� ��ȯ�ؼ� ���-> json���·� �����ϱ� ���� ISO 8601 ���� ���ڿ��� ��ȯ �� ����
  
       Data_Message(const nlohmann::json jsonStr)
       {
           id = jsonStr["id"];
           sender_id = jsonStr["sender_id"];
           receiver_id = jsonStr["receiver_id"];

           std::string utf8_message = jsonStr["message"];

           // UTF-8 �� WideChar (Unicode)
           int wideSize = MultiByteToWideChar(CP_UTF8, 0, utf8_message.c_str(), -1, NULL, 0);
           std::wstring wideStr(wideSize, 0);
           MultiByteToWideChar(CP_UTF8, 0, utf8_message.c_str(), -1, &wideStr[0], wideSize);

           // WideChar �� EUC-KR
           int eucSize = WideCharToMultiByte(949 /*EUC-KR �ڵ� ������*/, 0, wideStr.c_str(), -1, NULL, 0, NULL, NULL);
           std::string euc_message(eucSize, 0);
           WideCharToMultiByte(949, 0, wideStr.c_str(), -1, &euc_message[0], eucSize, NULL, NULL);
           message = euc_message;

           timestamp = jsonStr["timestamp"];
       }
  
   };  
}