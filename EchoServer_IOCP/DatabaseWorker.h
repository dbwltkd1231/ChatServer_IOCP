#pragma once
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <iostream>
#include <string>
#include<set>
#include<unordered_map>
#include<hiredis.h>

#include"DataStruct.h"
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
		bool DataLoadAsync(std::string ip, int port);
		TableType getTableType(const std::string& table);
		std::string StringConvert(std::wstring ws);
	private:
		SQLHENV mHenv;
		SQLHDBC mHdbc;
		SQLHSTMT mHstmt;
		redisContext* mRedis;

		std::set<std::string> mTableNameSet;
		std::unordered_map<std::string, TableType> mTableMap;

	};


}
