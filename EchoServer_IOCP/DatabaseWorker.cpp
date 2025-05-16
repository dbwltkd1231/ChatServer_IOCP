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
   }  

   void DatabaseWorker::Deinitalize()  
   {  
       // 연결 종료  
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
           std::cout << "테이블 이름: " << table << std::endl;
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
       }

       return true;
   }

   std::string DatabaseWorker::StringConvert(std::wstring ws)
   {
       std::string result = std::string(ws.begin(), ws.end());;

       return result;
   }
}


























//일단 어떻게 ~던 제가 판

 //1. 여자친구랑 지금다니는 회사에서 만났음.
 //2. 여자친구는 디렉터의 성희롱과 데이트강요, 거절하자 괴롭힘 등으로 비자발적 퇴사함, 그로인해 퇴사후에도 정신병원까지 다닐정도로 힘들어했음
 //3. 나는 남자라 디렉터한테 그런 괴롭힘은 받지않았고 당장 대구에 집도구한상태라 퇴사하지않고 계속 일을했음
 //4. 클라이언트 팀장이 진짜 답없어서 클라이언트 팀원들 프로젝트하던것 끝나고 전부퇴사함. 근데 이때 다들 나가니까 나 연봉올려줘서 일단 3개월만 더다니려고했음
 //5. 3개월차에 서버개발자로 보직변경하면서 팀을 바꿔줘서 개인적으로는 도움많이되고 잘 회사 다니는중(연봉도 또 20%정도 올려줄예정이라고 통지받음)

 //사건발생
 // 여자친구가 있었던일을 주변지인들에게 말했고, 그러한 사실이 입소문을 타서 최근에 입사한사람통해서 우리회사에 알려지게됨.
 // 디렉터는 그 소문이 전부 거짓이라고 화를 내고 허위사실유포로 고소하니 마니 말이 나오는중.
 // 내가 사귀고있는건 몰라도 회사다닐때 친했던건 알아서, (다른 회사사람들은 전부차단당해서 연락못하기 때문에) 나를통해 연락을 하려고함.
 // 나한테도 막 여자친구 욕을 하고있음. 자기는 잘해줬는데 뒤통수를쳤다느니, 배신자다, 거짓말하고 다닌다고 비방함.
 // 근데 디렉터가 여직원들한테 찝쩍대는건 은연중에 직원들 전부 다알고, 나는 여자친구한테 녹취록이나 카톡기록까지 다봐서 어이가 없음.(참 뻔뻔하네 라는생각?)

 // 고민
 // 성준 너였으면, 여자친구가 이런수모를 당하고있는데, 커리어나 금전적으로 이득을 얻기위해서 회사를 더 다닐거같아?