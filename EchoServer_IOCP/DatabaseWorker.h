#pragma once
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <iostream>
#include <string>
#include<set>
namespace Business
{
	class DatabaseWorker
	{
	public:
		DatabaseWorker()
		{
		}

		void Initalize();
		void Deinitalize();
		bool DataLoadAsync();
		std::string StringConvert(std::wstring ws);
	private:
		SQLHENV mHenv;
		SQLHDBC mHdbc;
		SQLHSTMT mHstmt;

		std::set<std::string> mTableNameSet;

	};


}
