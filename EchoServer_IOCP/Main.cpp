
/*
#include<oneTBB/include/tbb/concurrent_vector.h>
#include<flatbuffers/flatbuffers.h>
#include "MESSAGE_WRAPPER_generated.h"
*/

#include "EchoServer.h"


//#define BUFFER_SIZE 1024
#define MY_PORT_NUM 9090


int main()
{
	Business::EchoServer echoServer;
	echoServer.Initialize(MY_PORT_NUM);
	echoServer.Work();
}