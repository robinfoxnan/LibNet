#include <iostream>
#include "../include/EventLoopThreadPool.h"
#include "../include/IOEventLoop.h"
#include "../include/TcpConnection.h"
#include "../include/Util.h"

#include "../samples/HttpClient.h"

#include "../include/MyTimer.h"
#include "../include/CommonLog.h"

using namespace std;
using namespace  LibNet;

int main(int argc, char *argv[])
{
	LibNet::EventLoopThreadPool::instance().Init(1).start();
	TcpConnectionPtr conn = std::make_shared<TcpConnection>("127.0.0.1", 80);
	bool ret = conn->connectSyn(5);
	if (ret)
	{
		printf("connect ok");
	}
	else
	{
		printf("connect fail%d, %s", conn->getLastErr(), conn->getLastErrInfo().c_str());
	}

	ret = conn->connectSyn(5);
	if (ret)
	{
		printf("connect ok");
	}
	else
	{
		printf("connect fail%d, %s", conn->getLastErr(), conn->getLastErrInfo().c_str());
	}

	conn->closeSyn();

	conn.reset();   // must destruct before threadpool
	LibNet::EventLoopThreadPool::instance().stop();
	return 0;
}



