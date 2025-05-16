
/*
#include<oneTBB/include/tbb/concurrent_vector.h>
#include<flatbuffers/flatbuffers.h>
#include "MESSAGE_WRAPPER_generated.h"
*/

#include "EchoServer.h"


//#define BUFFER_SIZE 1024
#define MY_IP "127.0.0.1"
#define MY_PORT_NUM 9090
#define REDIS_PORT_NUM 6379


int main()
{
	Business::ChatServer echoServer;
	echoServer.Initialize(MY_IP, MY_PORT_NUM, REDIS_PORT_NUM);
	echoServer.Work();
}