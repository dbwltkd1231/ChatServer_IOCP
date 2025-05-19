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
       //std::ostringstream timestamp; // SQL의 datetime형식 ->  TIMESTAMP_STRUCT 구조체로 변환해서 사용-> json형태로 저장하기 위해 ISO 8601 형식 문자열로 변환 후 저장
       long long timestamp; // SQL의 datetime형식 ->  TIMESTAMP_STRUCT 구조체로 변환해서 사용-> json형태로 저장하기 위해 ISO 8601 형식 문자열로 변환 후 저장
  

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