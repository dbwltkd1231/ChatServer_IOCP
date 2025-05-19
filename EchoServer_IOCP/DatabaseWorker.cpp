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

        //ODBC ���� ����
        ret = SQLSetEnvAttr(mHenv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);

        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
            SQLGetDiagRecW(SQL_HANDLE_DBC, mHdbc, 1, sqlState, &nativeError, message, sizeof(message), &messageLength);
            std::wcout << L"ODBC ���� �߻�: " << message << L" (SQLState: " << sqlState << L")\n";
            return;
        }
        std::cout << "MSSQL Connect-4 Success" << std::endl;

        //���� �ڵ� ����
        ret = SQLAllocHandle(SQL_HANDLE_DBC, mHenv, &mHdbc);

        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
            SQLGetDiagRecW(SQL_HANDLE_DBC, mHdbc, 1, sqlState, &nativeError, message, sizeof(message), &messageLength);
            std::wcout << L"ODBC ���� �߻�: " << message << L" (SQLState: " << sqlState << L")\n";
            return;
        }
        std::cout << "MSSQL Connect-3 Success" << std::endl;

        //���� ���ڿ� ����
        //SQLWCHAR* connStr = (SQLWCHAR*)L"DRIVER={SQL Server};SERVER=DESKTOP-O5SU309\\SQLEXPRESS;DATABASE=MyChatServer;UID=sa;PWD=5760;";  -> windows�����϶� ��������ʴ´�.
        SQLWCHAR* connStr = (SQLWCHAR*)L"DRIVER={SQL Server};SERVER=DESKTOP-O5SU309\\SQLEXPRESS;DATABASE=MyChat;Trusted_Connection=yes;";


        //DB�� ����
        //->UNICODE ����
        ret = SQLDriverConnectW(mHdbc, NULL, connStr, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_COMPLETE);
        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
        {
            SQLGetDiagRecW(SQL_HANDLE_DBC, mHdbc, 1, sqlState, &nativeError, message, sizeof(message), &messageLength);
            std::wcout << L"ODBC ���� �߻�: " << message << L" (SQLState: " << sqlState << L")\n";
            return;
        }

        // ->>ANSI����
        //  if (SQLDriverConnect(mHdbc, NULL, connStr, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT) == SQL_SUCCESS) {  
        //      std::wcout << L"MSSQL ���� ����!\n";  
        //
        //      // SQL ������ ���� �ڵ� ����  
        //      SQLAllocHandle(SQL_HANDLE_STMT, mHdbc, &mHstmt);
        //  }  
        //  else  
        //  {  
        //      std::cout << "MSSQL Connect-1 Fail" << std::endl;
        //      std::wcout << L"���� ����!\n";  
        //      return;
        //  }  


     //���� ������ ���� ���� �ڵ� ����
        ret = SQLAllocHandle(SQL_HANDLE_STMT, mHdbc, &mHstmt);
        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
            std::cout << "ODBC ���� �߻�" << std::endl;
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
       redisFree(mRedis); // ���� ����

       // ���� ����  
       SQLFreeHandle(SQL_HANDLE_STMT, mHstmt);
       SQLDisconnect(mHdbc);
       SQLFreeHandle(SQL_HANDLE_DBC, mHdbc);
       SQLFreeHandle(SQL_HANDLE_ENV, mHenv);
   }  

   bool DatabaseWorker::DataLoadAndCaching(std::string ip, int port)
   {
       //���ڵ� ������ Redis ���� ���� �ʿ�
       mRedis = redisConnect(ip.c_str(), port);
       if (mRedis == NULL || mRedis->err)
       {
           std::cout << "Redis ���� ����!" << std::endl;
           return false;
       }
       std::cout << "Redis ���� ����!" << std::endl;


       SQLWCHAR* tableQuery = (SQLWCHAR*)L"SELECT TABLE_NAME FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_CATALOG = 'MyChat';";
       SQLRETURN ret = SQLExecDirectW(mHstmt, tableQuery, SQL_NTS);

       if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO)
       {
           std::cout << "���� ���� ����" << std::endl;

           SQLWCHAR tableName[256];
           while (SQLFetch(mHstmt) == SQL_SUCCESS) {
               SQLGetData(mHstmt, 1, SQL_C_WCHAR, tableName, sizeof(tableName), NULL);

               std::cout << "TableName : " << StringConvert(std::wstring(tableName)) << std::endl;
               mTableNameSet.insert(StringConvert(std::wstring(tableName)));
           }
       }
       else
       {
           std::cout << "���� ���� ���� �߻�" << std::endl;
       }

       SQLFreeStmt(mHstmt, SQL_CLOSE); // ���� ���� ��� �ݱ�
 
       for (auto table : mTableNameSet)
       {
   
           SQLFreeStmt(mHstmt, SQL_CLOSE); // ���� ���� ��� �ݱ�
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

                       std::time_t localTime = std::mktime(&timeinfo); // ���� �ð� ��ȯ
                       std::time_t utcTime = _mkgmtime(&timeinfo); // UTC �������� ��ȯ

                       nlohmann::json usersJson = {
                     {"id", std::wstring(id)}, {"password", std::wstring(password)}, {"created_at", utcTime}
                       };

                       SetCachedData("Users", StringConvert(std::wstring(id)), usersJson, 60);

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
                   //SQLWCHAR message[2048]; //- SQL_C_WCHAR�� UTF-16�� �д� ����̹Ƿ�, ���� �����Ͱ� UTF-8d�̸� �������ִ�.
                   SQLCHAR message[2048];//SQL_C_WCHAR�� UTF-16�� �д¹��.
                   TIMESTAMP_STRUCT timestamp;

                   while (SQLFetch(mHstmt) == SQL_SUCCESS)
                   {
                       SQLGetData(mHstmt, 1, SQL_C_LONG, &id, 0, NULL);
                       SQLGetData(mHstmt, 2, SQL_C_WCHAR, &sender_id, sizeof(sender_id), NULL);
                       SQLGetData(mHstmt, 3, SQL_C_WCHAR, &receiver_id, sizeof(receiver_id), NULL);

                       SQLLEN messageLen;
                       SQLGetData(mHstmt, 4, SQL_C_CHAR, message, sizeof(message), &messageLen);//SQL_C_CHAR

                       SQLGetData(mHstmt, 5, SQL_C_TYPE_TIMESTAMP, &timestamp, sizeof(TIMESTAMP_STRUCT), NULL);

                       // ��Ȯ�� ���̸�ŭ ��ȯ (NULL ���� ���Ե��� �ʴ� ��� ���)
                       std::string convertedMessage(reinterpret_cast<char*>(message), messageLen);

                       std::tm timeinfo = { timestamp.second, timestamp.minute, timestamp.hour, timestamp.day, timestamp.month - 1, timestamp.year - 1900 };

                       std::time_t localTime = std::mktime(&timeinfo); // ���� �ð� ��ȯ
                       std::time_t utcTime = _mkgmtime(&timeinfo); // UTC �������� ��ȯ

                       nlohmann::json messagesJson = {
                           {"id", id}, {"receiver_id", receiver_id}, {"sender_id", sender_id}, {"message",convertEUC_KRtoUTF8(convertedMessage)}, {"timestamp",utcTime}
                       };

                       SetCachedData("Messages", std::to_string(id), messagesJson, 60);

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

           SQLFreeStmt(mHstmt, SQL_CLOSE); // ���� ���� ��� �ݱ�

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

                   std::cout << "Į�� �̸� : " << StringConvert(std::wstring(columnName)) << ", ������ Ÿ��: " << StringConvert(std::wstring(dataType)) << std::endl;
               }
           }
           else
           {
               std::cout << table << " ���� �������� ����";
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

   void DatabaseWorker::SetCachedData(const std::string table, const std::string key, const nlohmann::json jsonData, int ttl)
   {
       std::string cacheKey = "table:" + table + ":" + key;

       for (auto& [field, value] : jsonData.items()) {
           std::string fieldValue;

           // ���ڿ� �����ʹ� �״�� ���� (dump() ��� X)
           if (value.is_string()) {
               fieldValue = value.get<std::string>();
           }
           else
           {
               fieldValue = value.dump(); // JSON ��ü�� ���ڴ� ����ȭ�Ͽ� ����
           }

           redisReply* reply = (redisReply*)redisCommand(mRedis, "HSET %s %s %s", cacheKey.c_str(), field.c_str(), fieldValue.c_str());
           if (reply == NULL) {
               std::cout << " Redis ���� ����!" << std::endl;
               return;
           }
           freeReplyObject(reply);
       }

       redisCommand(mRedis, "EXPIRE %s %d", cacheKey.c_str(), ttl);
       std::cout << "���� �Ϸ�: " << cacheKey << std::endl;
   }

   void DatabaseWorker::clearTableCache(const std::string table) 
   {
       std::string cacheKey = "table:" + table;

       redisReply* reply = (redisReply*)redisCommand(mRedis, "DEL %s", cacheKey.c_str());
       if (reply == NULL) {
           std::cout << "Redis ���� ����!" << std::endl;
           return;
       }

       std::cout << "�ʱ�ȭ �Ϸ�: " << cacheKey << " ������!" << std::endl;
       freeReplyObject(reply);
   }


   std::string DatabaseWorker::convertEUC_KRtoUTF8(const std::string& euc_kr_str)
   {
       // EUC-KR �� WideChar ��ȯ
       int wide_size = MultiByteToWideChar(949, 0, euc_kr_str.c_str(), -1, nullptr, 0);
       std::wstring wide_str(wide_size, 0);
       MultiByteToWideChar(949, 0, euc_kr_str.c_str(), -1, &wide_str[0], wide_size);

       // WideChar �� UTF-8 ��ȯ
       int utf8_size = WideCharToMultiByte(CP_UTF8, 0, wide_str.c_str(), -1, nullptr, 0, nullptr, nullptr);
       std::string utf8_str(utf8_size, 0);
       WideCharToMultiByte(CP_UTF8, 0, wide_str.c_str(), -1, &utf8_str[0], utf8_size, nullptr, nullptr);

       return utf8_str;
   }

   bool DatabaseWorker::IsKeyExists(const std::string table, const std::string key) {
       
       std::string cacheKey = table + ":" + key;
       redisReply* reply = (redisReply*)redisCommand(mRedis, "EXISTS %s", cacheKey.c_str());
       if (reply == NULL) {
           std::cout << "Redis Ȯ�� ����!" << std::endl;
           return false;
       }

       bool exists = reply->integer == 1;
       freeReplyObject(reply);
       return exists;
   }

   std::string DatabaseWorker::GetCachedData(const std::string table, const std::string key)
   {
       std::string cacheKey = "table:" + table + ":" + key;

       redisReply* reply = (redisReply*)redisCommand(mRedis, "HGETALL %s", cacheKey.c_str());
       if (reply == NULL || reply->type == REDIS_REPLY_NIL) {
           std::cout << "Redis�� ������ ����!" << std::endl;
           return "";
       }

       nlohmann::json jsonData;
       for (size_t i = 0; i < reply->elements; i += 2) {
           jsonData[reply->element[i]->str] = reply->element[i + 1]->str;
       }

       freeReplyObject(reply);
       return jsonData.dump();
   }
}