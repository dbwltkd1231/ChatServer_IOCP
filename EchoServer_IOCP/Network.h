#pragma once

#include<iostream>
#include<memory>
#include<functional>

#define NOMINMAX
#include <winsock2.h>
#include<MSWSock.h>
#include <windows.h>

#include<tbb/concurrent_map.h>

#include "Struct.h"
#include "Client.h"
#include "Session.h"
#include "LockFreeCircleQueue.h"


#define BUFFER_SIZE 1024

namespace IOCP
{
	class Network
	{
	private:
		std::unique_ptr<Session>* sessionArray;
		HANDLE mIOCPHandle = INVALID_HANDLE_VALUE;
		SOCKET mListenSocket = INVALID_SOCKET;
		LPFN_ACCEPTEX mAcceptExPointer = nullptr;

		std::shared_ptr<LockFreeCircleQueue<IOCP::CustomOverlapped*>> mContextPool;
		std::function<void(std::shared_ptr<IOCP::CustomOverlapped>)> mReceiveCallback;

		int mClientMax;
		int mNumThreads;
	public:
		Network()
		{
			SYSTEM_INFO sysInfo;
			GetSystemInfo(&sysInfo);
			mNumThreads = sysInfo.dwNumberOfProcessors * 1; // CPU �ھ� �� �� 2 �� �����ϴٰ� �ϴ��� ���� 1�� ����. 1 = 12
			sessionArray = new std::unique_ptr<Session>[mNumThreads];

			mContextPool = std::make_shared<LockFreeCircleQueue<IOCP::CustomOverlapped*>>();
			mClientMap = std::make_shared<tbb::concurrent_map<int, IOCP::Client*>>();
		}
		~Network()
		{

		}

		void Initialize(u_short port, int clientMax, int contextMax, std::function<void(std::shared_ptr<CustomOverlapped>)> receiveCallback);
		void SocketAccept();

		std::shared_ptr<tbb::concurrent_map<int, IOCP::Client*>> mClientMap;// ��Ʈ��ũ ������ Ŭ���̾�Ʈ�� index�� ���еǴ� �����̳�
	};

	void Network::Initialize(u_short port, int clientMax, int contextMax, std::function<void(std::shared_ptr<CustomOverlapped>)> receiveCallback)
	{
		mClientMax = clientMax;
		mReceiveCallback = receiveCallback;

		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)// 2.2 ������ Winsock �ʱ�ȭ
		{
			std::cout << "WSAStartup failed" << std::endl;
			return;
		}

		mListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (mListenSocket == INVALID_SOCKET)
		{
			std::cout << "listenSocket Create failed" << std::endl;
			WSACleanup();
			return;
		}

		sockaddr_in serverAddr{};
		serverAddr.sin_family = AF_INET; // IPv4
		serverAddr.sin_addr.s_addr = INADDR_ANY; // ��� IP �ּ� ����
		serverAddr.sin_port = htons(port);

		if (bind(mListenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)// ���Ͽ� �ּ� ���ε�
		{
			std::cout << "bind failed" << std::endl;
			closesocket(mListenSocket);
			WSACleanup();
			return;
		}
		std::cout << "bind success" << std::endl;

		if (listen(mListenSocket, SOMAXCONN) == SOCKET_ERROR)// ������ ��ŭ�� ������ŭ Ŭ���̾�Ʈ ���� ���, SOMAXCONN : ���� ������ ������ �� �ִ� ��� ���� ��û�� �ִ� ����
		{
			std::cout << "listen failed" << std::endl;
			closesocket(mListenSocket);
			WSACleanup();
			return;

		}
		std::cout << "listen success" << std::endl;

		// CreateIoCompletionPort�Լ��� IOCP �ڵ��� ������ �����ϴ�IOCP�� ������ ������ ��� ó�����ش�.
		mIOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 3);// IOCP �ڵ� ����.
		if (mIOCPHandle == NULL)
		{
			std::cout << "CreateIoCompletionPort failed" << std::endl;
			closesocket(mListenSocket);
			WSACleanup();
			return;
		}
		std::cout << "CreateIoCompletionPort success" << std::endl;

		// IOCP�� ���� ����.
		if (!CreateIoCompletionPort((HANDLE)mListenSocket, mIOCPHandle, 0, mNumThreads))
		{
			std::cout << "CreateIoCompletionPort failed" << std::endl;
			closesocket(mListenSocket);
			WSACleanup();
			return;
		}

		std::cout << "Iocp - Socket Connect" << std::endl;

		//AccpetEx�Լ��� winscok2�� Ȯ�������� ��������Ǿ� ���� �ʱ� ������, ��������� �Լ������͸� ���� �����;��Ѵ�.
		GUID guidAcceptEx = WSAID_ACCEPTEX;//WSAID_ACCEPTEX�� AcceptEx�Լ��� �ĺ��ϴ� GUID.
		DWORD bytesReceived;

		//WSAIoctl�Լ��� GUID�� ���� AcceptEx�Լ��� ã��, acceptExPointer�� �� �ּҸ� �����Ѵ�.
		if (WSAIoctl(mListenSocket, SIO_GET_EXTENSION_FUNCTION_POINTER,
			&guidAcceptEx, sizeof(guidAcceptEx), &mAcceptExPointer, sizeof(mAcceptExPointer),
			&bytesReceived, NULL, NULL) == SOCKET_ERROR)
		{
			std::cout << "WSAIoctl failed" << std::endl;
		}

		for (int i = 0;i < mNumThreads;++i)
		{
			sessionArray[i] = std::make_unique<Session>(mIOCPHandle, mContextPool, mClientMap);
			sessionArray[i]->Work();
		}

		for (int i = 0; i < mClientMax;++i)
		{
			SOCKET newSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);//WSA_FLAG_OVERLAPPED��  overlapped i/o�� ����ϱ� ���� �÷���.->�񵿱��۾� ���డ��������.
			auto socketSharedPtr = std::make_shared<SOCKET>(newSocket);
			Client* newClient = new Client(i, socketSharedPtr, mContextPool, mReceiveCallback);
			mClientMap->insert(std::make_pair(i, newClient));
			CreateIoCompletionPort((HANDLE)newSocket, mIOCPHandle, (ULONG_PTR)i, mNumThreads);// IOCP�� Ŭ���̾�Ʈ ���� ����
		}

		for (int i = 0;i < contextMax;++i)
		{
			IOCP::CustomOverlapped* newContext = new IOCP::CustomOverlapped();

			bool result = mContextPool->push(std::move(newContext));
			if (!result)
				std::cout << i << std::endl;
		}

		std::cout << "IOCP Port Ready Complete" << std::endl;
	}

	void Network::SocketAccept()
	{
		for (int i = 0; i < mClientMax;++i)
		{
			auto client = (*mClientMap)[i];
			client->AcceptReady(mListenSocket, mAcceptExPointer);
		}
	}
}
