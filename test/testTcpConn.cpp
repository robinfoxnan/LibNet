#include <iostream>
#include "../include/EventLoopThreadPool.h"
#include "../include/IOEventLoop.h"
#include "../include/TcpConnection.h"
#include "../include/Util.h"

using namespace std;
using namespace  LibNet;

static std::atomic<bool> bExit{ false };
TcpConnectionPtr  conn = nullptr;
std::string content;
char buffer[4096];
void onClose(const TcpConnectionPtr& connection)
{
	printf("server close socket\n");
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
    printf("recv:%zu, \n %s \n", sz, data);

	
}

void onWriteEnd(const TcpConnectionPtr &conn, const char * data, size_t sz, void *pVoid, int status)
{
	printf("send %zu \n", sz);
	//conn->stopWrite();
}

void onConnectReturn(const TcpConnectionPtr& conn)
{
	std::cout << "连接返回：" << conn->isConnected() << endl;

	content = "GET /test.php?id=1 HTTP/1.1\r\n"
		"Host: 10.128.6.122"
		"\r\n"
		"\r\n";

	//conn->shutdown(onClose);
	content = "GET /test.php?id=1 HTTP/1.1";
	content.append("\r\n");
		
	content.append("Host: 10.128.6.122");
	content.append("\r\n");
	content.append("\r\n");
	conn->send(content.c_str(), content.length(), 0);
}

int main(int argc, char *argv[])
{

	LibNet::EventLoopThreadPool::instance().Init(1);
	LibNet::EventLoopThreadPool::instance().start();

    string ip = "10.128.6.129";
	int port = 80;
	conn = std::make_shared<TcpConnection>(ip, 80);
	conn->setClientCallback(onAllocBuffer, onReadData, onWriteEnd, onClose);

	std::cout << "start to connect:" << ip << ":" <<port << endl;
	bool ret = conn->connectSyn(3);
		//conn->connect(onConnectReturn);
	if (ret == false)
	{
		printf("connect error\n");
		bExit = true;
	}
	else
	{
		printf("connect ok\n");
	}

	//Utils::sleepFor(2000);

	content =
		"GET /1.html HTTP/1.1\r\n"
		"Host: 127.0.0.1\r\n"
		"Connection: close\r\n"
		"Content-Type: text/html\r\n"
		"\r\n";

	conn->send(content.c_str(), content.length(), 0);
	
	while (!bExit)
	{
		Utils::sleepFor(1000);
	}

	conn = nullptr;
	LibNet::EventLoopThreadPool::instance().stop();

	return 0;
}



