#include <iostream>
#include "../include/EventLoopThreadPool.h"
#include "../include/IOEventLoop.h"
#include "../include/TcpConnection.h"
#include "../include/ApmUtil.h"

using namespace std;
using namespace  muduo;

static std::atomic<bool> bExit{ false };
TcpConnectionPtr  conn = nullptr;
std::string content;
char buffer[4096];
void onClose(const TcpConnectionPtr& connection)
{
	conn = nullptr;
	bExit = true;
}

void onAllocBuffer(const TcpConnectionPtr &conn, char * *data, size_t *sz, void **pVoid)
{
	*data = buffer;
	*sz = 4096;
	*pVoid = nullptr;
}

void onReadData(const TcpConnectionPtr &conn, char * data, size_t sz, void *pVoid)
{
	printf("recv:%u, \n %s \n", sz, data);

	
}

void onWriteEnd(const TcpConnectionPtr &conn, const char * data, size_t sz, void *pVoid, int status)
{
	printf("send %zu \n", sz);
	conn->stopWrite();
}

void onConnectReturn(const TcpConnectionPtr& conn)
{
	std::cout << "连接返回：" << conn->isConnected() << endl;


	//conn->shutdown(onClose);
	content = "GET / HTTP/1.1";
	content.append("\r\n");
		
	content.append("Host: 10.128.6.122");
	content.append("\r\n");
	content.append("\r\n");
	conn->send(content.c_str(), content.length(), 0);
}

int main(int argc, char *argv[])
{

	muduo::EventLoopThreadPool::instance().Init(1);
	muduo::EventLoopThreadPool::instance().start();

	string ip = "220.181.38.251";
	int port = 80;
	conn = std::make_shared<TcpConnection>(ip, 80);
	conn->setClientCallback(onAllocBuffer, onReadData, onWriteEnd, onClose);

	std::cout << "start to connect:" << ip << ":" <<port << endl;
	bool ret = conn->connect(onConnectReturn);

	Utils::sleepFor(2000);

	content = "GET /cache/hps/js/hps-1.1.js  HTTP/1.1";
	content.append("\r\n");

	content.append("Host: 10.128.6.122");
	content.append("\r\n");
	content.append("\r\n");
	conn->send(content.c_str(), content.length(), 0);
	
	while (!bExit)
	{
		Utils::sleepFor(1000);
	}

	

	
	//muduo::EventLoopThreadPool::instance().stop();

	return 0;
}



