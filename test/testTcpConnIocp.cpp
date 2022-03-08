#include <iostream>
#include "../include/EventLoopThreadPool.h"
#include "../include/IOEventLoop.h"
#include "../include/Util.h"

#include "../include/windows/TcpConnectionIocp.h"

using namespace std;
using namespace  LibNet;

static std::atomic<bool> bExit{ false };
TcpConnectionPtr  conn = nullptr;
std::string content;
char buffer[4096];
void onClose(const TcpConnectionPtr& connection)
{
	conn = nullptr;
	bExit = true;
	printf("server close socket");
}

void onAllocBuffer(const TcpConnectionPtr &conn, char * *data, size_t *sz, void **pVoid)
{
	*data = buffer;
	*sz = 4096;
	*pVoid = nullptr;
}


void onReadData(const TcpConnectionPtr &conn, char * data, size_t sz, void *pVoid)
{
	if (sz > 0)
	{
		data[sz] = 0;
		printf("recv: %zu bytes, body={ %s }\n", sz, data);
	}
	else
	{
		printf("server close\n");
	}

	
}

void onWriteEnd(const TcpConnectionPtr &conn, const char * data, size_t sz, void *pVoid, int status)
{
	printf("=====> send size = %zu \n", sz);
	//conn->stopWrite();
}

//void onConnectReturn(const TcpConnectionPtr& conn)
//{
//	std::cout << "连接返回：" << conn->isConnected() << endl;
//
//
//	//conn->shutdown(onClose);
//	content = "GET /1.html HTTP/1.1";
//	content.append("\r\n");
//		
//	content.append("Host: 10.128.6.122");
//	content.append("\r\n");
//	content.append("\r\n");
//	conn->send(content.c_str(), content.length(), 0);
//}

string getString(int id)
{
	stringstream ss;

	ss <<  "GET /test.php?id=" << id << " HTTP/1.1"; 
	ss << "\r\n";

	ss << "Host:127.0.0.1";
	ss << "\r\n";
	ss << "\r\n";

	return ss.str();
}

int main(int argc, char *argv[])
{

	LibNet::EventLoopThreadPool::instance().Init(1).start();

	string ip = "127.0.0.1";
	int port = 80;
	conn = std::make_shared<TcpConnectionIocp>(ip, 80);
	conn->setClientCallback(onAllocBuffer, onReadData, onWriteEnd, onClose);

	std::cout << "start to connect:" << ip << ":" <<port << endl;
	bool ret = conn->connectSyn(3);
	
	string url1 = getString(1);
	string url2 = getString(2);
	string url3 = getString(3);
	string url4 = getString(4);

	if (ret == false)
	{
		printf("connet error");
		goto err_exit;
	}

	//Utils::sleepFor(2000);

	conn->send(url1.c_str(), (int)url1.length(), 0);
	conn->send(url2.c_str(), (int)url2.length(), 0);
	conn->send(url3.c_str(), (int)url3.length(), 0);
	conn->send(url4.c_str(), (int)url4.length(), 0);
	
	while (!bExit)
	{
		Utils::sleepFor(1000);
	}


err_exit:
	conn = nullptr;
	LibNet::EventLoopThreadPool::instance().stop();

	return 0;
}



