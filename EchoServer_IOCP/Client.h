#pragma once

#include "Network.h"

namespace IOCP
{
	class Client
	{
	public:
		Client(int socket_id, std::shared_ptr<SOCKET> socket, std::shared_ptr<LockFreeCircleQueue<IOCP::CustomOverlapped*>> contextPool, std::function<void(std::shared_ptr<CustomOverlapped>)> receiveCallback)
		{
			mContextPool = contextPool;
			mSocket_ID = socket_id;
			mClientSocketPtr = socket;
			mReceiveCallback = receiveCallback;
		}
		~Client()
		{

		}

		bool AcceptReady(SOCKET& listenSocket, LPFN_ACCEPTEX& acceptExPointer);
		void ReceiveReady(); // context는 그때그때 가져오기
		void ReceiveFeedback(std::shared_ptr<IOCP::CustomOverlapped> copiedContext);
		void Send(char*& headerBuffer, char*& bodyBuffer, int bodyLen);
	private:
		int mSocket_ID;
		int mUSer_ID;
		std::function<void(std::shared_ptr<CustomOverlapped>)> mReceiveCallback;
		std::shared_ptr<LockFreeCircleQueue<IOCP::CustomOverlapped*>> mContextPool;
		std::shared_ptr<SOCKET> mClientSocketPtr = nullptr;

	};

	bool Client::AcceptReady(SOCKET& listenSocket, LPFN_ACCEPTEX& acceptExPointer)
	{
		//재활용된 소켓의 상태검증
		int errorCode;
		int errorCodeSize = sizeof(errorCode);
		getsockopt(*mClientSocketPtr, SOL_SOCKET, SO_ERROR, (char*)&errorCode, &errorCodeSize);
		if (errorCode != 0)
		{
			std::cerr << "Socket error detected: " << errorCode << std::endl;
			return false;
		}

		// 멀티플렉싱 방법 중 select, poll의 경우 소켓수 제한이 있어(windows 기본값 64개), 많은 클라이언트 연결을 처리하기 힘들다.
		// AcceptEx를 IOCP와 함께 사용하면, 대규모 동시연결을 효율적으로 처리할수있다.

		//여기서 completionKey가 올바른 값을 가지려면, OVERLAPPED 구조체가 정상적으로 설정되어 있어야 합니다.

		auto context = mContextPool->pop();
		context->Destruct();
		ZeroMemory(context, sizeof(IOCP::CustomOverlapped));// buffer의 내용을 0으로 채움 (해제 X), 이경우 포인터가 0이됨
		context->Construct();
		context->header = new IOCP::MessageHeader(mSocket_ID, 0, 0);
		context->operationType = IOCP::OperationType::OP_ACCEPT;

		DWORD bytesReceived = 0;
		bool result = acceptExPointer(listenSocket, *mClientSocketPtr, context->wsabuf[1].buf, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, &bytesReceived, (CustomOverlapped*)&(*context));
		//참고 : accept함수는 OVERLAPPED를 상속받지않은 구조체를 사용해도 되지만, acceptex함수는 비동기작업을 지원하기 위해 OVERLAPPED를 상속받은 구조체를 사용해야한다.
		//클라이언트 연결을 수락하는 과정이므로, 데이터를 수신하기 위한 특정 WSABUF 구조체를 요구하지 않음.

		if (result == SOCKET_ERROR)
		{
			int errorCode = WSAGetLastError();
			if (errorCode == WSA_IO_PENDING)
			{
				std::cout << "WSARecv 정상적으로 요청됨 (WSA_IO_PENDING)" << std::endl;
			}
			else
			{
				std::cerr << "WSARecv 실패! 오류 코드: " << errorCode << std::endl;
			}
		}

		std::cout << mSocket_ID << " 클라이언트소켓 연결준비\n";
	}

	void Client::ReceiveReady()
	{
		auto context = mContextPool->pop();
		context->Destruct();
		ZeroMemory(context, sizeof(IOCP::CustomOverlapped));// buffer의 내용을 0으로 채움 (해제 X), 이경우 포인터가 0이됨

		context->Construct();
		context->header = new IOCP::MessageHeader(mSocket_ID, 0, 0);
		context->operationType = IOCP::OperationType::OP_RECV;

		DWORD flags = 0;
		int result = WSARecv(*mClientSocketPtr, context->wsabuf, 2, nullptr, &flags, &*context, nullptr);// 비동기 읽기 요청
		if (result == SOCKET_ERROR)
		{
			int errorCode = WSAGetLastError();
			if (errorCode == WSA_IO_PENDING)
			{
				std::cout << "WSARecv 정상적으로 요청됨 (WSA_IO_PENDING)" << std::endl;
			}
			else
			{
				std::cerr << "WSARecv 실패! 오류 코드: " << errorCode << std::endl;
			}
		}

		std::cout << mSocket_ID << " 클라이언트소켓 Receive준비\n";
	}

	void Client::ReceiveFeedback(std::shared_ptr<IOCP::CustomOverlapped> copiedContext)
	{
		std::cout << "Receive Feedback!" << std::endl;
		mReceiveCallback(copiedContext);
	}

	void Client::Send(char*& headerBuffer, char*& bodyBuffer, int bodyLen)
	{
		auto context = mContextPool->pop();
		context->Destruct();
		ZeroMemory(context, sizeof(context));// buffer의 내용을 0으로 채움 (해제 X), 이경우 포인터가 0이됨
		context->Construct(headerBuffer, sizeof(IOCP::MessageHeader), bodyBuffer, bodyLen);
		context->operationType = IOCP::OperationType::OP_SEND;

		DWORD flags = 0;
		int result = WSASend(*mClientSocketPtr, context->wsabuf, 2, nullptr, flags, &(*context), nullptr);// 비동기 쓰기 요청
		if (result == SOCKET_ERROR)
		{
			int errorCode = WSAGetLastError();
			if (errorCode == WSA_IO_PENDING)
			{
				std::cout << "WSASend 정상적으로 요청됨 (WSA_IO_PENDING)" << std::endl;
			}
			else
			{
				std::cerr << "WSASend 실패! 오류 코드: " << errorCode << std::endl;
			}
		}

		//delete[] headerBuffer;
		//delete[] bodyBuffer; ->IOCP에실행됨
	}
}