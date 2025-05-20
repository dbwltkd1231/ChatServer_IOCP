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
		bool DataLoadAndCaching(std::string ip, int port);
		nlohmann::json GetCachedData(const std::string table, const std::string key);

		TableType getTableType(const std::string& table);
	private:
		SQLHENV mHenv;
		SQLHDBC mHdbc;
		SQLHSTMT mHstmt;
		redisContext* mRedis;

		std::set<std::string> mTableNameSet;
		std::unordered_map<std::string, TableType> mTableMap;

		void SetCachedData(const std::string table, const std::string key, std::string jsonString, int ttl);
		bool IsKeyExists(const std::string table, const std::string key);
		void clearTableCache(const std::string table);
	};


}
