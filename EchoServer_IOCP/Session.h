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
		DWORD bytesTransferred = 0; //bytesTransferred: ���۵� ����Ʈ ��,
		ULONG_PTR completionKey = 0; //completionKey: �Ϸ� Ű

		while (isActive)
		{
			BOOL result = GetQueuedCompletionStatus(mIocpHandle, &bytesTransferred, &completionKey, reinterpret_cast<LPOVERLAPPED*>(&overlapped), INFINITE);// IOCP���� �Ϸ�� �۾� ���

			if (result)
			{
				Client* client = nullptr;
				//auto client = reinterpret_cast<IOCP::Client*>(completionKey);//reinterpret_cast�� static_cast������ !

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
						//Ŭ���̾�Ʈ ��������.
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
	�����带 cpu �ھ��*2��ŭ �����.
	�� �����忡�� GetQueuedCompletionStatus�� �����Ѵ�.

	Ŭ���̾�Ʈ���� ����� ���ϸ� recv�� ȣ���Ѵ�.(recv������ �ٽ� �̾ recvȣ���Ѵ�. ������ ���涧���� �ݺ��Ѵ�.)

	�޼����� �����Ұ�� ������ �����忡�� GetQueuedCompletionStatus�� ���� �޼����� ó���Ҽ��ִ�.

	PostQueuedCompletionStatus�� ������ �ڱ��ڽ����� �޼����� ������ ����Ѵ�.

	*/
}
