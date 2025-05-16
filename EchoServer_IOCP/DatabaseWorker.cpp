#include "DatabaseWorker.h"  

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
       redisFree(mRedis); // ���� ����

       // ���� ����  
       SQLFreeHandle(SQL_HANDLE_STMT, mHstmt);
       SQLDisconnect(mHdbc);
       SQLFreeHandle(SQL_HANDLE_DBC, mHdbc);
       SQLFreeHandle(SQL_HANDLE_ENV, mHenv);
   }  

   bool DatabaseWorker::DataLoadAsync(std::string ip, int port)
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
                   long long created_at;

                   while (SQLFetch(mHstmt) == SQL_SUCCESS)
                   {
                       SQLGetData(mHstmt, 1, SQL_C_LONG, &id, 0, NULL);
                       SQLGetData(mHstmt, 2, SQL_C_WCHAR, password, sizeof(password), NULL);
                       SQLGetData(mHstmt, 5, SQL_C_SBIGINT, &created_at, 0, NULL);

                       Business::Data_User user{
                          StringConvert(std::wstring(id)),
                          StringConvert(std::wstring(password)),
                          std::chrono::system_clock::time_point(std::chrono::seconds(created_at))
                       };

                       std::string jsonData = user.toJson();
                       redisReply* reply = (redisReply*)redisCommand(mRedis, "SET User:%s %s", user.id.c_str(), jsonData.c_str());
                       freeReplyObject(reply);

                       std::cout << user.id << std::endl;
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
                   SQLWCHAR receiver_id[256];
                   SQLWCHAR message[1024];
                   long long timestamp;

                   while (SQLFetch(mHstmt) == SQL_SUCCESS)
                   {
                       SQLGetData(mHstmt, 1, SQL_C_LONG, &id, 0, NULL);
                       SQLGetData(mHstmt, 2, SQL_C_WCHAR, sender_id, sizeof(sender_id), NULL);
                       SQLGetData(mHstmt, 3, SQL_C_WCHAR, receiver_id, sizeof(receiver_id), NULL);
                       SQLGetData(mHstmt, 4, SQL_C_WCHAR, message, sizeof(message), NULL);
                       SQLGetData(mHstmt, 5, SQL_C_SBIGINT, &timestamp, 0, NULL);

                       Business::Data_Message messages{
                           id,
                           StringConvert(std::wstring(sender_id)),
                           StringConvert(std::wstring(receiver_id)),
                           StringConvert(std::wstring(message)),
                           std::chrono::system_clock::time_point(std::chrono::seconds(timestamp))
                       };

                       std::string jsonData = messages.toJson();
                       redisReply* reply = (redisReply*)redisCommand(mRedis, "SET User:%s %s", messages.sender_id.c_str(), jsonData.c_str());
                       freeReplyObject(reply);

                       std::cout << messages.sender_id << std::endl;
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
}


























//�ϴ� ��� ~�� ���� ��

 //1. ����ģ���� ���ݴٴϴ� ȸ�翡�� ������.
 //2. ����ģ���� ������ ����հ� ����Ʈ����, �������� ������ ������ ���ڹ��� �����, �׷����� ����Ŀ��� ���ź������� �ٴ������� ���������
 //3. ���� ���ڶ� �������� �׷� �������� �����ʾҰ� ���� �뱸�� �������ѻ��¶� ��������ʰ� ��� ��������
 //4. Ŭ���̾�Ʈ ������ ��¥ ���� Ŭ���̾�Ʈ ������ ������Ʈ�ϴ��� ������ ���������. �ٵ� �̶� �ٵ� �����ϱ� �� �����÷��༭ �ϴ� 3������ ���ٴϷ�������
 //5. 3�������� ���������ڷ� ���������ϸ鼭 ���� �ٲ��༭ ���������δ� �����̵ǰ� �� ȸ�� �ٴϴ���(������ �� 20%���� �÷��ٿ����̶�� ��������)

 //��ǹ߻�
 // ����ģ���� �־������� �ֺ����ε鿡�� ���߰�, �׷��� ����� �Լҹ��� Ÿ�� �ֱٿ� �Ի��ѻ�����ؼ� �츮ȸ�翡 �˷����Ե�.
 // ���ʹ� �� �ҹ��� ���� �����̶�� ȭ�� ���� ������������� ����ϴ� ���� ���� ��������.
 // ���� ��Ͱ��ִ°� ���� ȸ��ٴҶ� ģ�ߴ��� �˾Ƽ�, (�ٸ� ȸ�������� �������ܴ��ؼ� �������ϱ� ������) �������� ������ �Ϸ�����.
 // �����׵� �� ����ģ�� ���� �ϰ�����. �ڱ�� ������µ� ��������ƴٴ���, ����ڴ�, �������ϰ� �ٴѴٰ� �����.
 // �ٵ� ���Ͱ� ������������ ��½��°� �����߿� ������ ���� �پ˰�, ���� ����ģ������ ������̳� ī���ϱ��� �ٺ��� ���̰� ����.(�� �����ϳ� ��»���?)

 // ���
 // ���� �ʿ�����, ����ģ���� �̷����� ���ϰ��ִµ�, Ŀ��� ���������� �̵��� ������ؼ� ȸ�縦 �� �ٴҰŰ���?