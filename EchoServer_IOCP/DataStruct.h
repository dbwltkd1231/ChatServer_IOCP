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
       


       //  JSON 문자열을 파싱하여 구조체 멤버 변수 초기화
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
                   created_at = std::stoll(jsonData["created_at"].get<std::string>()); // 문자열 → 숫자로 변환
               }
               else
               {
                   created_at = jsonData.value("created_at", 0LL); // 숫자로 저장된 경우 그대로 사용
               }


           }
           catch (const std::exception& e) {
               std::cerr << "JSON 파싱 오류: " << e.what() << std::endl;
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
       //std::ostringstream timestamp; // SQL의 datetime형식 ->  TIMESTAMP_STRUCT 구조체로 변환해서 사용-> json형태로 저장하기 위해 ISO 8601 형식 문자열로 변환 후 저장
       long long timestamp; // SQL의 datetime형식 ->  TIMESTAMP_STRUCT 구조체로 변환해서 사용-> json형태로 저장하기 위해 ISO 8601 형식 문자열로 변환 후 저장
  
       Data_Message(std::string jsonStr)
       {
           // JSON 문자열을 파싱하여 구조체 멤버 변수 초기화
           try {
               nlohmann::json jsonData = nlohmann::json::parse(jsonStr);

               id = jsonData.value("id", 0);
               sender_id = jsonData.value("sender_id", "");
               receiver_id = jsonData.value("receiver_id", "");
               message = jsonData.value("message", "");
			   timestamp = jsonData.value("timestamp", 0);

           }
           catch (const std::exception& e) {
               std::cerr << "JSON 파싱 오류: " << e.what() << std::endl;
           }
       }
  
   };  
}