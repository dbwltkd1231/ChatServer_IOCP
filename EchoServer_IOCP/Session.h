#pragma once

#include<thread>
#include<memory>

#include "Network.h"

namespace IOCP
{
	class Session
	{
	public:
		Session(const HANDLE& handle,
			std::shared_ptr<LockFreeCircleQueue<IOCP::CustomOverlapped*>> contextPool,
			std::shared_ptr<tbb::concurrent_map<int, IOCP::Client*>> clientMap)
		{
			mIocpHandle = handle;
			isActive = true;
			mContextPool = contextPool;
			mClientMap = clientMap;
		}
		~Session()
		{
			isActive = false;
			mContextPool = nullptr;
			mClientMap = nullptr;
		}

		void Work();
	private:
		HANDLE mIocpHandle;
		bool isActive;
		std::thread mWorkThread;
		std::shared_ptr<LockFreeCircleQueue<IOCP::CustomOverlapped*>> mContextPool;
		std::shared_ptr<tbb::concurrent_map<int, IOCP::Client*>> mClientMap;

		void Process();
	};

	void Session::Work()
	{
		mWorkThread = std::thread(&Session::Process, this);
		mWorkThread.detach();
	}

	void Session::Process()
	{
		IOCP::CustomOverlapped* overlapped = nullptr;
		DWORD bytesTransferred = 0; //bytesTransferred: 전송된 바이트 수,
		ULONG_PTR completionKey = 0; //completionKey: 완료 키

		while (isActive)
		{
			BOOL result = GetQueuedCompletionStatus(mIocpHandle, &bytesTransferred, &completionKey, reinterpret_cast<LPOVERLAPPED*>(&overlapped), INFINITE);// IOCP에서 완료된 작업 대기

			if (result)
			{
				Client* client = nullptr;
				//auto client = reinterpret_cast<IOCP::Client*>(completionKey);//reinterpret_cast와 static_cast차이점 !

				if (bytesTransferred <= 0)
				{
					if (overlapped->operationType == IOCP::OperationType::OP_ACCEPT)
					{
						int key = overlapped->header->socket_id;

						auto finder = mClientMap->find(key);
						if (finder == mClientMap->end())
						{
							continue;
						}

						client = (*mClientMap)[key];
						client->ReceiveReady();
					}
					else
					{
						//클라이언트 접속종료.
						std::cout << "disonnectd Client : " << completionKey << std::endl;
					}
				}
				else
				{
					if (overlapped->operationType == IOCP::OperationType::OP_RECV)
					{

						client = (*mClientMap)[completionKey];

						std::cout << "Receive Check" << std::endl;
						std::shared_ptr<CustomOverlapped> copiedContext = std::make_shared<CustomOverlapped>();
						MessageHeader* receivedHeader = reinterpret_cast<MessageHeader*>(overlapped->wsabuf[0].buf);

						auto requeust_socket_id = completionKey;
						auto request_body_size = ntohl(receivedHeader->body_size);

						receivedHeader->socket_id = requeust_socket_id;
						copiedContext->SetHeader(*receivedHeader);
						copiedContext->SetBody(overlapped->wsabuf[1].buf, request_body_size);

						client->ReceiveFeedback(copiedContext);
						client->ReceiveReady();
					}
					else if (overlapped->operationType == IOCP::OperationType::OP_SEND)
					{
						std::cout << "Send Check" << std::endl;
					}
				}

				ZeroMemory(&overlapped, sizeof(overlapped));
				mContextPool->push(std::move(overlapped));
			}
		}
	}

	/*
	*
	쓰레드를 cpu 코어수*2만큼 만든다.
	그 쓰레드에서 GetQueuedCompletionStatus를 실행한다.

	클라이언트들은 연결된 소켓만 recv를 호출한다.(recv받으면 다시 이어서 recv호출한다. 연결이 끊길때까지 반복한다.)

	메세지를 수신할경우 랜덤한 쓰레드에서 GetQueuedCompletionStatus를 통해 메세지를 처리할수있다.

	PostQueuedCompletionStatus는 서버가 자기자신한테 메세지를 보낼때 사용한다.

	*/
}
