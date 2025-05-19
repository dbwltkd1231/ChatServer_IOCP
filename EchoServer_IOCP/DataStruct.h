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
       Data_User(const std::string& jsonStr) 
       {
           try
           {
               nlohmann::json jsonData = nlohmann::json::parse(jsonStr);

               auto array = jsonData.value("id", "");
			   for (auto& item : array)
			   {
				   std::cout << static_cast<char>(item) << std::endl;
			   }
               id = jsonData.value("id", "");
			   password = jsonData.value("password", "");

               if (jsonData["created_at"].is_string()) 
               {
                   created_at = std::stoll(jsonData["created_at"].get<std::string>()); // ���ڿ� �� ���ڷ� ��ȯ
               }
               else
               {
                   created_at = jsonData.value("created_at", 0LL); // ���ڷ� ����� ��� �״�� ���
               }


           }
           catch (const std::exception& e) {
               std::cerr << "JSON �Ľ� ����: " << e.what() << std::endl;
           }

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
       //std::ostringstream timestamp; // SQL�� datetime���� ->  TIMESTAMP_STRUCT ����ü�� ��ȯ�ؼ� ���-> json���·� �����ϱ� ���� ISO 8601 ���� ���ڿ��� ��ȯ �� ����
       long long timestamp; // SQL�� datetime���� ->  TIMESTAMP_STRUCT ����ü�� ��ȯ�ؼ� ���-> json���·� �����ϱ� ���� ISO 8601 ���� ���ڿ��� ��ȯ �� ����
  
       Data_Message(std::string jsonStr)
       {
           // JSON ���ڿ��� �Ľ��Ͽ� ����ü ��� ���� �ʱ�ȭ
           try {
               nlohmann::json jsonData = nlohmann::json::parse(jsonStr);

               id = jsonData.value("id", 0);
               sender_id = jsonData.value("sender_id", "");
               receiver_id = jsonData.value("receiver_id", "");
               message = jsonData.value("message", "");
			   timestamp = jsonData.value("timestamp", 0);

           }
           catch (const std::exception& e) {
               std::cerr << "JSON �Ľ� ����: " << e.what() << std::endl;
           }
       }
  
   };  
}