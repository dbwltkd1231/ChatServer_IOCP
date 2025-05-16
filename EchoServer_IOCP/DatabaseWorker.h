#pragma once
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <iostream>

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
	private:
		SQLHENV mHenv;
		SQLHDBC mHdbc;
		SQLHSTMT mHstmt;
	};


}
