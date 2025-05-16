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
   }  

   void DatabaseWorker::Deinitalize()  
   {  
       // ���� ����  
       SQLFreeHandle(SQL_HANDLE_STMT, mHstmt);
       SQLDisconnect(mHdbc);
       SQLFreeHandle(SQL_HANDLE_DBC, mHdbc);
       SQLFreeHandle(SQL_HANDLE_ENV, mHenv);
   }  

   bool DatabaseWorker::DataLoadAsync()
   {
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
           std::cout << "���̺� �̸�: " << table << std::endl;
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
       }

       return true;
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