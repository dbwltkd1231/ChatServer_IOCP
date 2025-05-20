#include "DatabaseWorker.h"  
#include <iostream>
#include <string>



namespace Business  
{  
    void DatabaseWorker::Initalize()
    {
        SQLWCHAR sqlState[6], message[256];
        SQLINTEGER nativeError;
        SQLSMALLINT messageLength;

        SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &mHenv);

        //ODBC 버전 설정
        ret = SQLSetEnvAttr(mHenv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);

        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
            SQLGetDiagRecW(SQL_HANDLE_DBC, mHdbc, 1, sqlState, &nativeError, message, sizeof(message), &messageLength);
            std::wcout << L"ODBC 오류 발생: " << message << L" (SQLState: " << sqlState << L")\n";
            return;
        }
        std::cout << "MSSQL Connect-4 Success" << std::endl;

        //연결 핸들 생성
        ret = SQLAllocHandle(SQL_HANDLE_DBC, mHenv, &mHdbc);

        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
            SQLGetDiagRecW(SQL_HANDLE_DBC, mHdbc, 1, sqlState, &nativeError, message, sizeof(message), &messageLength);
            std::wcout << L"ODBC 오류 발생: " << message << L" (SQLState: " << sqlState << L")\n";
            return;
        }
        std::cout << "MSSQL Connect-3 Success" << std::endl;

        //연결 문자열 생성
        //SQLWCHAR* connStr = (SQLWCHAR*)L"DRIVER={SQL Server};SERVER=DESKTOP-O5SU309\\SQLEXPRESS;DATABASE=MyChatServer;UID=sa;PWD=5760;";  -> windows인증일땐 사용하지않는다.
        SQLWCHAR* connStr = (SQLWCHAR*)L"DRIVER={SQL Server};SERVER=DESKTOP-O5SU309\\SQLEXPRESS;DATABASE=MyChat;Trusted_Connection=yes;";


        //DB에 연결
        //->UNICODE 버전
        ret = SQLDriverConnectW(mHdbc, NULL, connStr, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_COMPLETE);
        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
        {
            SQLGetDiagRecW(SQL_HANDLE_DBC, mHdbc, 1, sqlState, &nativeError, message, sizeof(message), &messageLength);
            std::wcout << L"ODBC 오류 발생: " << message << L" (SQLState: " << sqlState << L")\n";
            return;
        }

        // ->>ANSI버전
        //  if (SQLDriverConnect(mHdbc, NULL, connStr, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT) == SQL_SUCCESS) {  
        //      std::wcout << L"MSSQL 연결 성공!\n";  
        //
        //      // SQL 실행을 위한 핸들 설정  
        //      SQLAllocHandle(SQL_HANDLE_STMT, mHdbc, &mHstmt);
        //  }  
        //  else  
        //  {  
        //      std::cout << "MSSQL Connect-1 Fail" << std::endl;
        //      std::wcout << L"연결 실패!\n";  
        //      return;
        //  }  


     //쿼리 실행을 위한 문장 핸들 생성
        ret = SQLAllocHandle(SQL_HANDLE_STMT, mHdbc, &mHstmt);
        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
            std::cout << "ODBC 오류 발생" << std::endl;
            return;
        }

        std::cout << "MSSQL Connect Success" << std::endl;

        mTableMap = {
                    {"Messages", TableType::Messages},
                    {"Users", TableType::Users}
                    };
    }

   void DatabaseWorker::Deinitalize()  
   {  
       clearTableCache("Users");
       clearTableCache("Messages");
       redisFree(mRedis); // 연결 해제

       // 연결 종료  
       SQLFreeHandle(SQL_HANDLE_STMT, mHstmt);
       SQLDisconnect(mHdbc);
       SQLFreeHandle(SQL_HANDLE_DBC, mHdbc);
       SQLFreeHandle(SQL_HANDLE_ENV, mHenv);
   }  

   bool DatabaseWorker::DataLoadAndCaching(std::string ip, int port)
   {
       //이코드 실행전 Redis 서버 실행 필요
       mRedis = redisConnect(ip.c_str(), port);
       if (mRedis == NULL || mRedis->err)
       {
           std::cout << "Redis 연결 실패!" << std::endl;
           return false;
       }
       std::cout << "Redis 연결 성공!" << std::endl;


       SQLWCHAR* tableQuery = (SQLWCHAR*)L"SELECT TABLE_NAME FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_CATALOG = 'MyChat';";
       SQLRETURN ret = SQLExecDirectW(mHstmt, tableQuery, SQL_NTS);

       if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO)
       {
           std::cout << "쿼리 실행 성공" << std::endl;

           SQLWCHAR tableName[256];
           while (SQLFetch(mHstmt) == SQL_SUCCESS) {
               SQLGetData(mHstmt, 1, SQL_C_WCHAR, tableName, sizeof(tableName), NULL);

               std::cout << "TableName : " << Business::Utility::StringConvert(std::wstring(tableName)) << std::endl;
               mTableNameSet.insert(Business::Utility::StringConvert(std::wstring(tableName)));
           }
       }
       else
       {
           std::cout << "쿼리 실행 오류 발생" << std::endl;
       }

       SQLFreeStmt(mHstmt, SQL_CLOSE); // 이전 쿼리 결과 닫기
 
       for (auto table : mTableNameSet)
       {
   
           SQLFreeStmt(mHstmt, SQL_CLOSE); // 이전 쿼리 결과 닫기
           auto tableType = getTableType(table);

           switch(tableType)
           {
           case TableType::Unknown:
           {
               std::cout << "Unknown" << std::endl;
               break;
           }
           case TableType::Users:
           {
               std::cout << "Users" << std::endl;

               std::wstring queryStr = L"SELECT id, password, created_at FROM " + std::wstring(table.begin(), table.end());
               SQLWCHAR* dataQuery = (SQLWCHAR*)queryStr.c_str();
               SQLRETURN dataRet = SQLExecDirectW(mHstmt, dataQuery, SQL_NTS);

               if (dataRet == SQL_SUCCESS || dataRet == SQL_SUCCESS_WITH_INFO)
               {
                   SQLWCHAR id[16];
                   SQLWCHAR password[50];
                   TIMESTAMP_STRUCT created_at;

                   while (SQLFetch(mHstmt) == SQL_SUCCESS)
                   {
                       SQLGetData(mHstmt, 1, SQL_C_WCHAR, &id, sizeof(id), NULL);
                       SQLGetData(mHstmt, 2, SQL_C_WCHAR, password, sizeof(password), NULL);
                       SQLGetData(mHstmt, 3, SQL_C_TYPE_TIMESTAMP, &created_at, sizeof(TIMESTAMP_STRUCT), NULL);

                       std::tm timeinfo = { created_at.second, created_at.minute, created_at.hour, created_at.day, created_at.month - 1, created_at.year - 1900 };

                       std::time_t localTime = std::mktime(&timeinfo); // 로컬 시간 변환
                       std::time_t utcTime = _mkgmtime(&timeinfo); // UTC 기준으로 변환

					   auto usersJson = Data_User::toJson(Business::Utility::WstringToUTF8(id), Business::Utility::WstringToUTF8(password), utcTime);
                       std::string jsonString = usersJson.dump(); // JSON을 문자열로 변환
                       SetCachedData(table, Business::Utility::WstringToUTF8(id), jsonString, 60); // 60초 TTL 설정);

                   }
               }

               break;
           }
           case TableType::Messages:
           {
               std::cout << "Messages" << std::endl;
               std::wstring queryStr = L"SELECT id, sender_id, receiver_id, message, timestamp FROM " + std::wstring(table.begin(), table.end());
               SQLWCHAR* dataQuery = (SQLWCHAR*)queryStr.c_str();

               SQLRETURN dataRet = SQLExecDirectW(mHstmt, dataQuery, SQL_NTS);
               if (dataRet == SQL_SUCCESS || dataRet == SQL_SUCCESS_WITH_INFO)
               {
                   int id;
                   SQLWCHAR sender_id[16];
                   SQLWCHAR receiver_id[16];
                   //SQLWCHAR message[2048]; //- SQL_C_WCHAR은 UTF-16을 읽는 방식이므로, 만약 데이터가 UTF-8d이면 깨질수있다.
                   SQLCHAR message[2048];//SQL_C_WCHAR은 UTF-16을 읽는방식.
                   TIMESTAMP_STRUCT timestamp;

                   while (SQLFetch(mHstmt) == SQL_SUCCESS)
                   {
                       SQLGetData(mHstmt, 1, SQL_C_LONG, &id, 0, NULL);
                       SQLGetData(mHstmt, 2, SQL_C_WCHAR, &sender_id, sizeof(sender_id), NULL);
                       SQLGetData(mHstmt, 3, SQL_C_WCHAR, &receiver_id, sizeof(receiver_id), NULL);

                       SQLLEN messageLen;
                       SQLGetData(mHstmt, 4, SQL_C_CHAR, message, sizeof(message), &messageLen);//SQL_C_CHAR
                       SQLGetData(mHstmt, 5, SQL_C_TYPE_TIMESTAMP, &timestamp, sizeof(TIMESTAMP_STRUCT), NULL);

                       std::tm timeinfo = { timestamp.second, timestamp.minute, timestamp.hour, timestamp.day, timestamp.month - 1, timestamp.year - 1900 };
                       std::time_t localTime = std::mktime(&timeinfo); // 로컬 시간 변환
                       std::time_t utcTime = _mkgmtime(&timeinfo); // UTC 기준으로 변환
                    
                       std::string convertedMessage = Business::Utility::ConvertEUC_KRtoUTF8(std::string(reinterpret_cast<char*>(message), messageLen));
                       // 정확한 길이만큼 변환 (NULL 종료 포함되지 않는 경우 대비),
                       //nlohmann::json 라이브러리는 내부적으로 UTF-8 인코딩을 강제

                       nlohmann::json messagesJson = {
                           {"id", id},
                           {"receiver_id", Business::Utility::WstringToUTF8(receiver_id)},
                           {"sender_id", Business::Utility::WstringToUTF8(sender_id)},
                           {"message", convertedMessage},
                           {"timestamp", utcTime}
                       };
                       std::string jsonString = messagesJson.dump(); // JSON을 문자열로 변환
					   SetCachedData(table, std::to_string(id), jsonString, 60); // 60초 TTL 설정);
                   }
               }
               break;
           }
           default:
               break;
           }

           SQLFreeStmt(mHstmt, SQL_CLOSE); // 이전 쿼리 결과 닫기
       }

       return true;
   }

   TableType DatabaseWorker::getTableType(const std::string& table)
   {
       auto it = mTableMap.find(table);
       return (it != mTableMap.end()) ? it->second : TableType::Unknown;
   }


   void DatabaseWorker::SetCachedData(const std::string table, const std::string key, std::string jsonString, int ttl)
   {
       std::string cacheKey = "table:" + table + ":" + key;
       redisReply* reply = (redisReply*)redisCommand(mRedis, "SET %s %s EX %d", cacheKey.c_str(), jsonString.c_str(), ttl);
   }

   void DatabaseWorker::clearTableCache(const std::string table) 
   {
       std::string cacheKey = "table:" + table;

       redisReply* reply = (redisReply*)redisCommand(mRedis, "DEL %s", cacheKey.c_str());
       if (reply == NULL) {
           std::cout << "Redis 삭제 오류!" << std::endl;
           return;
       }

       std::cout << "초기화 완료: " << cacheKey << " 삭제됨!" << std::endl;
       freeReplyObject(reply);
   }

   bool DatabaseWorker::IsKeyExists(const std::string table, const std::string key) {
       
       std::string cacheKey = table + ":" + key;
       redisReply* reply = (redisReply*)redisCommand(mRedis, "EXISTS %s", cacheKey.c_str());
       if (reply == NULL) {
           std::cout << "Redis 확인 오류!" << std::endl;
           return false;
       }

       bool exists = reply->integer == 1;
       freeReplyObject(reply);
       return exists;
   }

   nlohmann::json DatabaseWorker::GetCachedData(const std::string table, const std::string key)
   {

       std::string cacheKey = "table:" + table + ":" + key;

       redisReply* reply = (redisReply*)redisCommand(mRedis, "GET %s", cacheKey.c_str());
       if (reply != NULL && reply->type == REDIS_REPLY_STRING)
       {
           std::string jsonString = reply->str;
           nlohmann::json parsedJson = nlohmann::json::parse(jsonString);
           freeReplyObject(reply);
           return parsedJson;
       }

	   return ""; // 데이터가 없거나 오류 발생 시 빈 문자열 반환
   }
}