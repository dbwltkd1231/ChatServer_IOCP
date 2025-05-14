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
		void ReceiveReady(); // context�� �׶��׶� ��������
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
		//��Ȱ��� ������ ���°���
		int errorCode;
		int errorCodeSize = sizeof(errorCode);
		getsockopt(*mClientSocketPtr, SOL_SOCKET, SO_ERROR, (char*)&errorCode, &errorCodeSize);
		if (errorCode != 0)
		{
			std::cerr << "Socket error detected: " << errorCode << std::endl;
			return false;
		}

		// ��Ƽ�÷��� ��� �� select, poll�� ��� ���ϼ� ������ �־�(windows �⺻�� 64��), ���� Ŭ���̾�Ʈ ������ ó���ϱ� �����.
		// AcceptEx�� IOCP�� �Բ� ����ϸ�, ��Ը� ���ÿ����� ȿ�������� ó���Ҽ��ִ�.

		//���⼭ completionKey�� �ùٸ� ���� ��������, OVERLAPPED ����ü�� ���������� �����Ǿ� �־�� �մϴ�.

		auto context = mContextPool->pop();
		context->Destruct();
		ZeroMemory(context, sizeof(IOCP::CustomOverlapped));// buffer�� ������ 0���� ä�� (���� X), �̰�� �����Ͱ� 0�̵�
		context->Construct();
		context->header = new IOCP::MessageHeader(mSocket_ID, 0, 0);
		context->operationType = IOCP::OperationType::OP_ACCEPT;

		DWORD bytesReceived = 0;
		bool result = acceptExPointer(listenSocket, *mClientSocketPtr, context->wsabuf[1].buf, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, &bytesReceived, (CustomOverlapped*)&(*context));
		//���� : accept�Լ��� OVERLAPPED�� ��ӹ������� ����ü�� ����ص� ������, acceptex�Լ��� �񵿱��۾��� �����ϱ� ���� OVERLAPPED�� ��ӹ��� ����ü�� ����ؾ��Ѵ�.
		//Ŭ���̾�Ʈ ������ �����ϴ� �����̹Ƿ�, �����͸� �����ϱ� ���� Ư�� WSABUF ����ü�� �䱸���� ����.

		if (result == SOCKET_ERROR)
		{
			int errorCode = WSAGetLastError();
			if (errorCode == WSA_IO_PENDING)
			{
				std::cout << "WSARecv ���������� ��û�� (WSA_IO_PENDING)" << std::endl;
			}
			else
			{
				std::cerr << "WSARecv ����! ���� �ڵ�: " << errorCode << std::endl;
			}
		}

		std::cout << mSocket_ID << " Ŭ���̾�Ʈ���� �����غ�\n";
	}

	void Client::ReceiveReady()
	{
		auto context = mContextPool->pop();
		context->Destruct();
		ZeroMemory(context, sizeof(IOCP::CustomOverlapped));// buffer�� ������ 0���� ä�� (���� X), �̰�� �����Ͱ� 0�̵�

		context->Construct();
		context->header = new IOCP::MessageHeader(mSocket_ID, 0, 0);
		context->operationType = IOCP::OperationType::OP_RECV;

		DWORD flags = 0;
		int result = WSARecv(*mClientSocketPtr, context->wsabuf, 2, nullptr, &flags, &*context, nullptr);// �񵿱� �б� ��û
		if (result == SOCKET_ERROR)
		{
			int errorCode = WSAGetLastError();
			if (errorCode == WSA_IO_PENDING)
			{
				std::cout << "WSARecv ���������� ��û�� (WSA_IO_PENDING)" << std::endl;
			}
			else
			{
				std::cerr << "WSARecv ����! ���� �ڵ�: " << errorCode << std::endl;
			}
		}

		std::cout << mSocket_ID << " Ŭ���̾�Ʈ���� Receive�غ�\n";
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
		ZeroMemory(context, sizeof(context));// buffer�� ������ 0���� ä�� (���� X), �̰�� �����Ͱ� 0�̵�
		context->Construct(headerBuffer, sizeof(IOCP::MessageHeader), bodyBuffer, bodyLen);
		context->operationType = IOCP::OperationType::OP_SEND;

		DWORD flags = 0;
		int result = WSASend(*mClientSocketPtr, context->wsabuf, 2, nullptr, flags, &(*context), nullptr);// �񵿱� ���� ��û
		if (result == SOCKET_ERROR)
		{
			int errorCode = WSAGetLastError();
			if (errorCode == WSA_IO_PENDING)
			{
				std::cout << "WSASend ���������� ��û�� (WSA_IO_PENDING)" << std::endl;
			}
			else
			{
				std::cerr << "WSASend ����! ���� �ڵ�: " << errorCode << std::endl;
			}
		}

		//delete[] headerBuffer;
		//delete[] bodyBuffer; ->IOCP�������
	}
}