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
       std::string id;  
       std::string password;  
       std::chrono::system_clock::time_point created_at; // datetime  
       
       std::string toJson() const {  
           nlohmann::json j; // Ensure the nlohmann namespace is correctly resolved  
           j["id"] = id;  
           j["password"] = password;  
           j["created_at"] = std::chrono::duration_cast<std::chrono::seconds>(created_at.time_since_epoch()).count();  
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
       std::string receiver_id;  
       std::string message;  
       std::chrono::system_clock::time_point timestamp; // datetime  

       std::string toJson() const {  
           nlohmann::json j; // Ensure the nlohmann namespace is correctly resolved  
           j["id"] = id;  
           j["sender_id"] = sender_id;  
           j["receiver_id"] = receiver_id;  
           j["message"] = message;  
           j["timestamp"] = std::chrono::duration_cast<std::chrono::seconds>(timestamp.time_since_epoch()).count();  
           return j.dump();  
       }  
   };  
}