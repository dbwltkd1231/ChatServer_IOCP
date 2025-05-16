#include "DatabaseWorker.h"  

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
       //SQLWCHAR* connStr = (SQLWCHAR*)L"DRIVER={ODBC Driver 17 for SQL Server};SERVER=DESKTOP-O5SU309\\SQLEXPRESS;DATABASE=MyChat;Trusted_Connection=yes;";
       // 
       //DB에 연결
       ret = SQLDriverConnectW(mHdbc, NULL, connStr, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_COMPLETE);//->UNICODE 버전
       if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
           SQLGetDiagRecW(SQL_HANDLE_DBC, mHdbc, 1, sqlState, &nativeError, message, sizeof(message), &messageLength);
           std::wcout << L"ODBC 오류 발생: " << message << L" (SQLState: " << sqlState << L")\n";
           return;
       }
       std::cout << "MSSQL Connect-2 Success" << std::endl;

     //쿼리 실행을 위한 문장 핸들 생성
     ret = SQLAllocHandle(SQL_HANDLE_STMT, mHdbc, &mHstmt);
     if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
         std::cout << "ODBC 오류 발생" << std::endl;
         return;
     }

     std::cout << "MSSQL Connect Success" << std::endl;
   
     SQLWCHAR* query = (SQLWCHAR*)L"SELECT TABLE_NAME FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_CATALOG = 'MyChat';";

     ret = SQLExecDirectW(mHstmt, query, SQL_NTS);
     if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO)
     {
         std::cout << "쿼리 실행 성공" << std::endl;
     
         SQLWCHAR tableName[256];
         while (SQLFetch(mHstmt) == SQL_SUCCESS) {
             SQLGetData(mHstmt, 1, SQL_C_WCHAR, tableName, sizeof(tableName), NULL);
             std::wcout << L"테이블 이름: " << tableName << std::endl;

             std::wstring ws(tableName);
             std::string str(ws.begin(), ws.end());
             std::cout << str << std::endl;
         }
     }
     else {
         std::wcout << L"쿼리 실행 오류 발생!" << std::endl;
         std::cout << "쿼리 실행 오류 발생" << std::endl;
     }


   //  // MSSQL 연결  ->ANSI버전
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


   }  

   void DatabaseWorker::Deinitalize()  
   {  
       // 연결 종료  
       SQLFreeHandle(SQL_HANDLE_STMT, mHstmt);
       SQLDisconnect(mHdbc);
       SQLFreeHandle(SQL_HANDLE_DBC, mHdbc);
       SQLFreeHandle(SQL_HANDLE_ENV, mHenv);
   }  
}
