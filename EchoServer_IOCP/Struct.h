#pragma once
#include<memory>

#include "Network.h"

#define BUFFER_SIZE 1024

namespace IOCP
{
    enum OperationType
    {
        OP_DEFAULT = 0,
        OP_ACCEPT = 1,// connectex �ڵ尡 ��Ʈ��ũ�ڵ�κ��ε�, header�� ���Ұ�� �ش� ������Ʈ�� flatbuffer�� ���Ӽ��� ����⶧���� ��ü enum���� �����Ѵ�.
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
        //��� or OVERLAPPED overlapped;// OVERLAPPED�� ������ IOCP���� �񵿱� �۾��� ������ �� �����Ƿ�, �ݵ�� �����ؾ� �մϴ�.
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
            std::cout << "CustomOverlapped �Ҹ���ȣ��" << std::endl;
        }

        // ���� ������
        CustomOverlapped(const CustomOverlapped& other)
        {
            // ��� ���� (���� ����)
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
            �̴� WSARecv()�� wsabuf.len�� ������ �ִ� ũ��� �ν��ϱ� ������,
            wsabuf[0].len = sizeof(MessageHeader);�� �����ϸ� ���� �ʿ��� ��ŭ�� �а� �Ǵ� ������!
            �ᱹ, ��� ũ�⸦ �ùٸ��� ���������� ������ ��ü ���� ũ��(1024)��ŭ ó���Ϸ��� �ϸ鼭 �����Ͱ� ������ ������ �߻��ߴ� �ų׿�.
            �̷� �������� �κ��� ��Ʈ��ũ ���α׷��ֿ��� ���� �߿��� ��Ҷ�� �� �ٽ� �� �� Ȯ���� �� �־����!
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
            //    delete header;  // ���� ��� ���� (�޸� ���� ����)
            //}

            header = new MessageHeader(headerData); // �������� �Ҵ�

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
