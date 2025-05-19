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
       redisFree(mRedis); // 연결 해제

       // 연결 종료  
       SQLFreeHandle(SQL_HANDLE_STMT, mHstmt);
       SQLDisconnect(mHdbc);
       SQLFreeHandle(SQL_HANDLE_DBC, mHdbc);
       SQLFreeHandle(SQL_HANDLE_ENV, mHenv);
   }  

   bool DatabaseWorker::DataLoadAsync(std::string ip, int port)
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

               std::cout << "TableName : " << StringConvert(std::wstring(tableName)) << std::endl;
               mTableNameSet.insert(StringConvert(std::wstring(tableName)));
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

                       Business::Data_User user{
                          StringConvert(std::wstring(id)),
                          StringConvert(std::wstring(password)),
                          utcTime
                       };

                      // std::string jsonData = user.toJson();
                      // redisReply* reply = (redisReply*)redisCommand(mRedis, "SET User:%s %s", user.id.c_str(), jsonData.c_str());
                      // freeReplyObject(reply);

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

                       // 정확한 길이만큼 변환 (NULL 종료 포함되지 않는 경우 대비)
                       std::string convertedMessage(reinterpret_cast<char*>(message), messageLen);

                       std::tm timeinfo = { timestamp.second, timestamp.minute, timestamp.hour, timestamp.day, timestamp.month - 1, timestamp.year - 1900 };

                       std::time_t localTime = std::mktime(&timeinfo); // 로컬 시간 변환
                       std::time_t utcTime = _mkgmtime(&timeinfo); // UTC 기준으로 변환

                       Business::Data_Message messages{
                           id,
                           StringConvert(std::wstring(sender_id)),
                           StringConvert(std::wstring(receiver_id)),
                           convertedMessage,
                           utcTime
                           //std::chrono::system_clock::time_point(std::chrono::milliseconds(timestamp))//- timestamp 값이 초 단위이면 std::chrono::seconds, 아니라 밀리초(millisecond) 단위

                       };

                      // std::string jsonData = messages.toJson();
                      // redisReply* reply = (redisReply*)redisCommand(mRedis, "SET Messages:%s %s", messages.id, jsonData.c_str());
                      // freeReplyObject(reply);

                   }
               }
               break;
           }
           default:
               break;
           }

           SQLFreeStmt(mHstmt, SQL_CLOSE); // 이전 쿼리 결과 닫기

           /*
           std::wstring queryStr = L"SELECT COLUMN_NAME, DATA_TYPE FROM INFORMATION_SCHEMA.COLUMNS WHERE TABLE_NAME = '" + std::wstring(table.begin(), table.end()) + L"'";

           SQLWCHAR* columnsQuery = (SQLWCHAR*)queryStr.c_str();
           SQLRETURN colRet = SQLExecDirectW(mHstmt, columnsQuery, SQL_NTS);
           if (colRet == SQL_SUCCESS || colRet == SQL_SUCCESS_WITH_INFO) {
               SQLWCHAR columnName[256], dataType[256];
               while (SQLFetch(mHstmt) == SQL_SUCCESS)
               {
                   SQLGetData(mHstmt, 1, SQL_C_WCHAR, columnName, sizeof(columnName), NULL);
                   SQLGetData(mHstmt, 2, SQL_C_WCHAR, dataType, sizeof(dataType), NULL);

                   std::cout << "칼럼 이름 : " << StringConvert(std::wstring(columnName)) << ", 데이터 타입: " << StringConvert(std::wstring(dataType)) << std::endl;
               }
           }
           else
           {
               std::cout << table << " 정보 가져오기 실패";
           }
           */
       }

       return true;
   }

   TableType DatabaseWorker::getTableType(const std::string& table)
   {
       auto it = mTableMap.find(table);
       return (it != mTableMap.end()) ? it->second : TableType::Unknown;
   }

   std::string DatabaseWorker::StringConvert(std::wstring ws)
   {
       std::string result = std::string(ws.begin(), ws.end());;

       return result;
   }
}