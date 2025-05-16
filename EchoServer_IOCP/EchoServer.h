#pragma once

#include<iostream>
#include<thread>
#include<string>

#define NOMINMAX
#include <winsock2.h>
#include<MSWSock.h>
#include <windows.h>

#include<tbb/concurrent_map.h>

#include "LockFreeCircleQueue.h"
#include "Network.h"
#include "Protocol.h"
#include "DatabaseWorker.h"

#include "flatbuffers/flatbuffers.h"
#include"MESSAGE_WRAPPER_generated.h"

namespace Business
{
	class ChatServer
	{
	public:
		void Initialize(std::string ip, int serverPortNum, int redisPortNum);
		void Work();
		void Read(std::shared_ptr<IOCP::CustomOverlapped> context);
	private:
		IOCP::Network network;
		bool mServerOn;
		std::thread mWorkThread;
		Business::DatabaseWorker mDatabaseWorker;

		tbb::concurrent_map<int, std::shared_ptr<IOCP::Client>> mID_ClientMap; // connectID로 관리되는 클라이언트

		constexpr static int clientMax = 5;//100
		constexpr static int contextMax = 10;//200

		void Process();
	};

	void ChatServer::Initialize(std::string ip, int serverPortNum, int redisPortNum)
	{
		mDatabaseWorker.Initalize();
		mDatabaseWorker.DataLoadAsync(ip, redisPortNum);

		auto receiveCallback = std::function<void(std::shared_ptr<IOCP::CustomOverlapped>)>(
			std::bind(&ChatServer::Read, this, std::placeholders::_1)
		);

		network.Initialize(serverPortNum, clientMax, contextMax, receiveCallback);
		network.SocketAccept();

		mServerOn = true;
	}

	void ChatServer::Work()
	{
		mWorkThread = std::thread(&ChatServer::Process, this);
		if (mWorkThread.joinable())
		{
			mWorkThread.join();
		}
	}

	void ChatServer::Process()
	{
		while (true)//mServerOn
		{
			continue;
		}
	}

	void ChatServer::Read(std::shared_ptr<IOCP::CustomOverlapped> context)//복사본으로 전달
	{
		int socket_id = context->header->socket_id;//completion key
		auto request_body_size = ntohl(context->header->body_size);
		auto request_contents_type = ntohl(context->header->contents_type);

		auto messageType = static_cast<protocol::MessageContent>(request_contents_type);//request_contents_type

		auto finder = network.mClientMap->find(socket_id);
		if (finder == network.mClientMap->end())
		{
			std::cout << "client find error! socketIndex: " << socket_id << std::endl;
			return;
		}

		auto client = (*network.mClientMap)[socket_id];

		//auto verifier = flatbuffers::Verifier(reinterpret_cast<const uint8_t*>(overlapped->wsabuf[1].buf), request_body_size); // 버퍼와 크기를 사용하여 Verifier 객체 생성
		//if (verifier.Check(false))
		//{
		//	std::cout << "Message FIlter verifier check false" << std::endl;
		//	return;
		//}

		switch (messageType)
		{
		case protocol::MessageContent_NONE:
			break;
		case protocol::MessageContent_REQUEST_CONNECT:
		{
			std::cout << "Receive : MessageContent_REQUEST_CONNECT" << std::endl;
			auto message_wrapper = flatbuffers::GetRoot<protocol::REQUEST_CONNECT>(context->wsabuf[1].buf); // FlatBuffers 버퍼에서 루트 객체 가져오기

			flatbuffers::Verifier verifier(reinterpret_cast<const uint8_t*>(context->wsabuf[1].buf), request_body_size);
			if (!verifier.VerifyBuffer<protocol::REQUEST_CONNECT>())
			{
				std::cout << "REQUEST_CONNECT error " << std::endl;
				return;
			}

			///////////////////////////////////////////////////////////////////////////

			mID_ClientMap.insert(std::make_pair(socket_id, client));
			std::cout << "REQUEST_CONNECT SUCCESS! socketID: " << socket_id << std::endl;

			auto builder = Create_Response_Connect(true);

			auto data = builder.GetBufferPointer();
			auto size = builder.GetSize();

			char* message = reinterpret_cast<char*>(data);

			auto response_socket_id = htonl(socket_id);
			auto response_body_size = htonl(size);
			auto response_contents_type = htonl(static_cast<uint32_t>(protocol::MessageContent_RESPONSE_CONNECT));

			IOCP::MessageHeader messageHeader(response_socket_id, response_body_size, response_contents_type);
			IOCP::CustomOverlapped newOverlap;

			char* headerBuffer = new char[sizeof(IOCP::MessageHeader)];
			memcpy(headerBuffer, &messageHeader, sizeof(IOCP::MessageHeader));

			client->Send(headerBuffer, message, size);
			///////////////////////////////////


			//Network::CustomOverlapped newOverlap;
			//newOverlap.SetHeader(messageHeader);
			//newOverlap.SetBody(message, size);

			break;
		}
		case protocol::MessageContent_RESPONSE_CONNECT:
		{
			std::cout << "MessageContent_RESPONSE_CONNECT" << std::endl;
			break;
		}
		case protocol::MessageContent_REQUEST_LOGIN:
		{
			auto message_wrapper = flatbuffers::GetRoot<protocol::REQUEST_LOGIN>(context->wsabuf[1].buf);
			std::cout << "Request Login -> ID: " << message_wrapper->user_id() << "Password: " << message_wrapper->password() << std::endl;


			break;
		}
		case protocol::MessageContent_RESPONSE_LOGIN:
		{
			std::cout << "MessageContent_RESPONSE_LOGIN" << std::endl;
			break;
		}
		case protocol::MessageContent_REQUEST_CREATE_ACCOUNT:
		{
			auto message_wrapper = flatbuffers::GetRoot<protocol::REQUEST_CREATE_ACCOUNT>(context->wsabuf[1].buf);

			flatbuffers::Verifier verifier(reinterpret_cast<const uint8_t*>(context->wsabuf[1].buf), request_body_size);
			if (!verifier.VerifyBuffer<protocol::REQUEST_CREATE_ACCOUNT>()) {
				std::cout << "REQUEST_CREATE_ACCOUNT error ?" << std::endl;
				return;
			}

			std::cout << "Request CREATE_ACCOUNT -> ID: " << message_wrapper->user_id()->str() << " Password: " << message_wrapper->password()->str() << std::endl;

			auto builder = Create_Response_Create_Account(message_wrapper->user_id()->str(), message_wrapper->password()->str());

			auto data = builder.GetBufferPointer();
			auto size = builder.GetSize();

			char* message = reinterpret_cast<char*>(data);

			auto response_socket_id = htonl(socket_id);
			auto response_body_size = htonl(size);
			auto response_contents_type = htonl(static_cast<uint32_t>(protocol::MESSAGETYPE_RESPONSE_CREATE_ACCOUNT));

			IOCP::MessageHeader messageHeader(response_socket_id, response_body_size, response_contents_type);
			IOCP::CustomOverlapped newOverlap;

			char* headerBuffer = new char[sizeof(IOCP::MessageHeader)];
			memcpy(headerBuffer, &messageHeader, sizeof(IOCP::MessageHeader));

			client->Send(headerBuffer, message, size);

			break;
		}
		case protocol::MessageContent_RESPONSE_CREATE_ACCOUNT:
		{
			std::cout << "MessageContent_RESPONSE_CREATE_ACCOUNT" << std::endl;
			break;
		}
		/*

		case protocol::MessageContent_REQUEST_LOGOUT:
		{
			std::cout << "MessageContent_REQUEST_LOGOUT" << std::endl;

			auto message_wrapper = flatbuffers::GetRoot<protocol::REQUEST_LOGOUT>(context->wsabuf[1].buf);

			std::string id = std::string(*message_wrapper->id());
			std::string password = std::string(*message_wrapper->password());
			break;
		}
		case protocol::MessageContent_RESPONSE_LOGOUT:
		{
			std::cout << "MessageContent_RESPONSE_LOGOUT" << std::endl;

			auto message_wrapper = flatbuffers::GetRoot<protocol::RESPONSE_LOGOUT>(context->wsabuf[1].buf);

			std::string id = std::string(*message_wrapper->id());
			bool feedback = message_wrapper->feedback();
			break;
		}
		*/
		default:
			break;
		}
	}
}
