#pragma once

#include "flatbuffers/flatbuffers.h"
#include"MESSAGE_WRAPPER_generated.h"

namespace Business
{
	static flatbuffers::FlatBufferBuilder Create_Response_Connect(bool isSuccess)
	{
		flatbuffers::FlatBufferBuilder builder;
		builder.Finish(protocol::CreateRESPONSE_CONNECT(builder, isSuccess));

		return builder;
	}

	static void Create_Response_Login(std::string id, std::string password)
	{
		;
	}

	static flatbuffers::FlatBufferBuilder Create_Response_Create_Account(std::string id, std::string password)
	{
		flatbuffers::FlatBufferBuilder builder;
		auto userID = builder.CreateString(id);
		builder.Finish(protocol::CreateRESPONSE_CREATE_ACCOUNT(builder, userID, true));

		return builder;
	}

	static void Create_Response_Logout()
	{

	}
}