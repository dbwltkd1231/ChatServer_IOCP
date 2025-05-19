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
       //std::ostringstream timestamp; // SQL�� datetime���� ->  TIMESTAMP_STRUCT ����ü�� ��ȯ�ؼ� ���-> json���·� �����ϱ� ���� ISO 8601 ���� ���ڿ��� ��ȯ �� ����
       long long timestamp; // SQL�� datetime���� ->  TIMESTAMP_STRUCT ����ü�� ��ȯ�ؼ� ���-> json���·� �����ϱ� ���� ISO 8601 ���� ���ڿ��� ��ȯ �� ����
  

       std::string toJson() const {  
           nlohmann::json j; // Ensure the nlohmann namespace is correctly resolved  
           j["id"] = id;  
           j["sender_id"] = sender_id;  
           j["receiver_id"] = receiver_id;  
           j["message"] = message;  
           j["timestamp"] = timestamp;
           return j.dump();  
       }  
   };  
}