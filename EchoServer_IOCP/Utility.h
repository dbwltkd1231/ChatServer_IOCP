#pragma once
#include <string>

#include <windows.h>
#include <nlohmann/json.hpp> 


namespace Business
{
	class Utility
	{
	public:
		static std::string ConvertEUC_KRtoUTF8(const std::string& euc_kr_str);
		static std::string ConvertUTF8toEUC_KR(const std::string& utf8_str);
		static std::string WstringToUTF8(const std::wstring& wstr);
		static std::string StringConvert(std::wstring ws);
	};
}

