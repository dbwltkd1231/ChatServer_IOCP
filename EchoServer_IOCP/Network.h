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
			mNumThreads = sysInfo.dwNumberOfProcessors * 1; // CPU 코어 수 × 2 가 적정하다고 하던데 나는 1로 설정. 1 = 12
			sessionArray = new std::unique_ptr<Session>[mNumThreads];

			mContextPool = std::make_shared<LockFreeCircleQueue<IOCP::CustomOverlapped*>>();
			mClientMap = std::make_shared<tbb::concurrent_map<int, IOCP::Client*>>();
		}
		~Network()
		{

		}

		void Initialize(u_short port, int clientMax, int contextMax, std::function<void(std::shared_ptr<CustomOverlapped>)> receiveCallback);
		void SocketAccept();

		std::shared_ptr<tbb::concurrent_map<int, IOCP::Client*>> mClientMap;// 네트워크 내에서 클라이언트를 index로 구분되는 컨테이너
	};

	void Network::Initialize(u_short port, int clientMax, int contextMax, std::function<void(std::shared_ptr<CustomOverlapped>)> receiveCallback)
	{
		mClientMax = clientMax;
		mReceiveCallback = receiveCallback;

		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)// 2.2 버전의 Winsock 초기화
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
		serverAddr.sin_addr.s_addr = INADDR_ANY; // 모든 IP 주소 수신
		serverAddr.sin_port = htons(port);

		if (bind(mListenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)// 소켓에 주소 바인딩
		{
			std::cout << "bind failed" << std::endl;
			closesocket(mListenSocket);
			WSACleanup();
			return;
		}
		std::cout << "bind success" << std::endl;

		if (listen(mListenSocket, SOMAXCONN) == SOCKET_ERROR)// 설정한 만큼의 개수만큼 클라이언트 연결 대기, SOMAXCONN : 서버 소켓이 수용할 수 있는 대기 연결 요청의 최대 개수
		{
			std::cout << "listen failed" << std::endl;
			closesocket(mListenSocket);
			WSACleanup();
			return;

		}
		std::cout << "listen success" << std::endl;

		// CreateIoCompletionPort함수는 IOCP 핸들을 생성과 존재하는IOCP와 소켓을 연결을 모두 처리해준다.
		mIOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 3);// IOCP 핸들 생성.
		if (mIOCPHandle == NULL)
		{
			std::cout << "CreateIoCompletionPort failed" << std::endl;
			closesocket(mListenSocket);
			WSACleanup();
			return;
		}
		std::cout << "CreateIoCompletionPort success" << std::endl;

		// IOCP와 소켓 연결.
		if (!CreateIoCompletionPort((HANDLE)mListenSocket, mIOCPHandle, 0, mNumThreads))
		{
			std::cout << "CreateIoCompletionPort failed" << std::endl;
			closesocket(mListenSocket);
			WSACleanup();
			return;
		}

		std::cout << "Iocp - Socket Connect" << std::endl;

		//AccpetEx함수는 winscok2의 확장기능으로 직접선언되어 있지 않기 때문에, 명시적으로 함수포인터를 통해 가져와야한다.
		GUID guidAcceptEx = WSAID_ACCEPTEX;//WSAID_ACCEPTEX는 AcceptEx함수를 식별하는 GUID.
		DWORD bytesReceived;

		//WSAIoctl함수는 GUID를 통해 AcceptEx함수를 찾고, acceptExPointer에 그 주소를 저장한다.
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
			SOCKET newSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);//WSA_FLAG_OVERLAPPED는  overlapped i/o를 사용하기 위한 플래그.->비동기작업 수행가능해진다.
			auto socketSharedPtr = std::make_shared<SOCKET>(newSocket);
			Client* newClient = new Client(i, socketSharedPtr, mContextPool, mReceiveCallback);
			mClientMap->insert(std::make_pair(i, newClient));
			CreateIoCompletionPort((HANDLE)newSocket, mIOCPHandle, (ULONG_PTR)i, mNumThreads);// IOCP와 클라이언트 소켓 연결
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
