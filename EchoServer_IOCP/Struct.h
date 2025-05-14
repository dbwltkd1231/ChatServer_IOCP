#pragma once
#include<memory>

#include "Network.h"

#define BUFFER_SIZE 1024

namespace IOCP
{
    enum OperationType
    {
        OP_DEFAULT = 0,
        OP_ACCEPT = 1,// connectex 코드가 네트워크코드부분인데, header를 통할경우 해당 프로젝트의 flatbuffer와 종속성이 생기기때문에 자체 enum으로 구분한다.
        OP_RECV = 2,
        OP_SEND = 3,
    };

#pragma pack(push, 1)
    struct MessageHeader
    {
        uint32_t socket_id;
        uint32_t body_size;
        uint32_t contents_type;

        MessageHeader(uint32_t socketID, uint32_t boydSize, uint32_t contentsType) : socket_id(socketID), body_size(boydSize), contents_type(contentsType)
        {

        }

        MessageHeader(const MessageHeader& other) : socket_id(other.socket_id), body_size(other.body_size), contents_type(other.contents_type)
        {
        }
    };
#pragma pack(pop)
    struct CustomOverlapped :OVERLAPPED
    {
        //상속 or OVERLAPPED overlapped;// OVERLAPPED가 없으면 IOCP에서 비동기 작업을 추적할 수 없으므로, 반드시 포함해야 합니다.
        WSABUF wsabuf[2];
        MessageHeader* header;
        OperationType operationType;

        CustomOverlapped()
        {
            header = nullptr;
            operationType = OperationType::OP_DEFAULT;
        }

        ~CustomOverlapped()
        {
            std::cout << "CustomOverlapped 소멸자호출" << std::endl;
        }

        // 복사 생성자
        CustomOverlapped(const CustomOverlapped& other)
        {
            // 헤더 복사 (깊은 복사)
            if (other.header)
            {
                header = new MessageHeader(*other.header);
            }
            else {
                header = nullptr;
            }

            operationType = other.operationType;

            if (other.header)
            {
                header = new MessageHeader(*other.header);
                wsabuf[0].buf = reinterpret_cast<char*>(header);
                wsabuf[0].len = sizeof(MessageHeader);
            }
            else
            {
                header = nullptr;
            }

            if (other.wsabuf[1].buf && other.wsabuf[1].len > 0) {
                wsabuf[1].buf = new char[other.wsabuf[1].len];
                memcpy(wsabuf[1].buf, other.wsabuf[1].buf, other.wsabuf[1].len);
                wsabuf[1].len = other.wsabuf[1].len;
            }
            else
            {
                wsabuf[1].buf = nullptr;
                wsabuf[1].len = 0;
            }
        }

        void Construct()
        {
            wsabuf[0].buf = new char[BUFFER_SIZE];
            wsabuf[0].len = sizeof(MessageHeader);

            wsabuf[1].buf = new char[BUFFER_SIZE];
            wsabuf[1].len = BUFFER_SIZE;

            /*
            이는 WSARecv()가 wsabuf.len을 수신할 최대 크기로 인식하기 때문에,
            wsabuf[0].len = sizeof(MessageHeader);로 변경하면 실제 필요한 만큼만 읽게 되는 것이죠!
            결국, 헤더 크기를 올바르게 지정해주지 않으면 전체 버퍼 크기(1024)만큼 처리하려고 하면서 데이터가 깨지는 문제가 발생했던 거네요.
            이런 디테일한 부분이 네트워크 프로그래밍에서 정말 중요한 요소라는 걸 다시 한 번 확인할 수 있었어요!
            */
        }

        void Construct(char* headrBuffer, ULONG headerLen, char* bodyBuffer, ULONG bodyLen)
        {
            wsabuf[0].buf = new char[headerLen];
            memcpy(wsabuf[0].buf, headrBuffer, headerLen);
            wsabuf[0].len = headerLen;

            wsabuf[1].buf = new char[bodyLen];
            memcpy(wsabuf[1].buf, bodyBuffer, bodyLen);
            wsabuf[1].len = bodyLen;

        }
        void Destruct()
        {
            //if (wsabuf[0].buf != nullptr)
            //{
            //    delete[] wsabuf[0].buf;
            //}
            //
            //if (wsabuf[1].buf != nullptr)
            //{
            //    delete[] wsabuf[1].buf;
            //}

            header = nullptr;

            wsabuf[0].buf = nullptr;
            wsabuf[1].buf = nullptr;
        }

        void SetHeader(const MessageHeader& headerData)
        {
            //if (header != nullptr)
            //{
            //    delete header;  // 기존 헤더 삭제 (메모리 누수 방지)
            //}

            header = new MessageHeader(headerData); // 동적으로 할당

            wsabuf[0].buf = reinterpret_cast<char*>(header);
            wsabuf[0].len = sizeof(MessageHeader);
        }

        void SetBody(char* data, size_t dataSize)
        {
            // if (wsabuf[1].buf != nullptr)
            // {
            //     delete[] wsabuf[1].buf;
            // }

            wsabuf[1].buf = new char[dataSize];
            memcpy(wsabuf[1].buf, data, dataSize);
            wsabuf[1].len = dataSize;
        }

        // CustomOverlapped()
        // {
        //     wsabuf[0].buf = reinterpret_cast<char*>(header);
        //     wsabuf[0].len = sizeof(MessageHeader);
        //     wsabuf[1].buf = bodyBuffer;
        //     wsabuf[1].len = BUFFER_SIZE;
        // }
    };
}
