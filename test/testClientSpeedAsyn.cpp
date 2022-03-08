#include <iostream>
#include <functional>
#include "../include/EventLoopThreadPool.h"
#include "../include/IOEventLoop.h"
#include "../include/TcpConnection.h"
#include "../include/Util.h"

#include "../http/HttpClient.h"

#include "../include/MyTimer.h"
#include "../include/CommonLog.h"

using namespace std;
using namespace  LibNet;

static std::atomic<bool> bExit{ false };

const int nThreads = 3;
const int nClients = 9;

// curent send count
atomic<int64_t> recvCount(0);
atomic<int64_t> sendCount(0);
atomic<int64_t> errorCount(0);

// save last second count with these
int64_t recvCountLast(0);
int64_t sendCountLast(0);
int64_t errorCountLast(0);



using HttpClientPtr = std::shared_ptr<HttpClient>;
std::vector<HttpClientPtr> clients;

string body = R"(======================================)";

void onEvent(HttpClient* client, std::shared_ptr<Request> &req, ResponsePtr & res)
{
	if (res->error == Error::Success)
	{
		recvCount++;
		//printf("===>code = %d, body = %s \n", res->code, res->body.c_str());
	}
	else if (res->error == Error::ConnectionReset)
	{
		//printf("---> reset url = %s\n", req->path.c_str());
	}
	else if (res->error == Error::Timeout)
	{
		errorCount++;
		printf("---> time out url = %s\n", req->path.c_str());
		// skip the timeout cmd
	}
	else
	{
		printf("---> error = %d\n", res->error);
	}
	sendCount++;
	if (client)
		client->GetAsyn("/hi", Headers{});
}

void init()
{
	LibNet::EventLoopThreadPool::instance().Init(nThreads).start();

    string ip = "10.128.6.129";
	int port = 80;
	stringstream ss;
	for (int i = 0; i < 1000; i++)
	{
		ss << "=";
	}
	body = ss.str();

	for (int i = 0; i < nClients; i++)
	{
		HttpClientPtr client = std::make_shared<HttpClient>(ip, port);
		client->setCmdTimeout(3);
		HttpEventCallbackPipe cb = std::bind(&onEvent, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
		client->enableAsynCmd(cb);

		clients.emplace_back(std::move(client));
	}
}

void sendStart()
{
	for (size_t i=0; i<clients.size(); i++)
	{
		sendCount++;
		clients[i]->GetAsyn("/hi", Headers{});
	}

}

void clean()
{
	clients.clear();
	LibNet::EventLoopThreadPool::instance().stop();
}

void logSpeed()
{
	while (bExit == false)
	{
		Utils::sleepFor(1000);
		int64_t deltaSend = sendCount - sendCountLast;
		sendCountLast += deltaSend;

		int64_t deltaRecv = recvCount - recvCountLast;
		recvCountLast += deltaRecv;

		printf("send all = %lld, recv all= %lld, send speed= %lld, recv speed =%lld\n", sendCount.load(), recvCount.load(), deltaSend, deltaRecv);
	}
}

int main(int argc, char *argv[])
{
	init();
	sendStart();
	logSpeed();
	clean();
	return 0;
}



