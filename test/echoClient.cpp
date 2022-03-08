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

static std::atomic<bool> bExit{ false };

string body = R"({ "age": 5 })";

// extern list for consumer
std::deque<string> sendList;
std::deque<string> errorList;
std::mutex mutexList;

void  resendInPipeLine(string & path)
{
	std::lock_guard<std::mutex> guard(mutexList);
	sendList.emplace_back(path);
}

void testPipelining()
{
	// must init thread pool first;
	LibNet::EventLoopThreadPool::instance().Init(1).start();

	string ip = "220.181.38.251";
	ip = "10.128.6.236";
	int port = 80;
	HttpClient httpClient(ip, port);
	httpClient.setCmdTimeout(2);

	int count = 0;
	int N = 30;
	for (int i = 0; i < N; i++)
	{
		char path[260];
		snprintf(path, 260, "/test.php?id=%d", i + 1);
		sendList.emplace_back(path);
	}

	
	HttpEventCallbackPipe cb =
		[&](std::shared_ptr<Request> &req, ResponsePtr & res)
	{
		
		if (res->error == Error::Success)
		{
			count++;
			printf("current count = %d\n", count);
			printf("=====> recv response %d: ", count);
			printf("%s \n", res->body.c_str());
		}
		else if (res->error == Error::ConnectionReset)
		{
			string info = res->getErrorInfo();
			printf("req %s is cancaled because of %s\n", req->path.c_str(), info.c_str());
			resendInPipeLine(req->path);
		}
		else if (res->error == Error::Timeout)
		{
			printf("###req %s is timeout, close and resend \n", req->path.c_str());
			resendInPipeLine(req->path);
		}

	};
	httpClient.enablePipeline(cb);
	
	
	Timer  timer;
	timer.start();
	while (true)
	{
		if (count >= N)
			break;

		string url;
		{
			std::lock_guard<std::mutex> guard(mutexList);

			if ( sendList.empty())
			{
				Utils::sleepFor(10);
				continue;
			}
		
			url = sendList[0];
			sendList.pop_front();	
		}

		FORMAT_DEBUG("start to send %s", url.c_str());
		int64_t id;
		//id = httpClient.GetInPipeline(path, Headers{ {"Content-Type", "Application/json"} });
		id = httpClient.PostInPipeline(url, Headers{ {"Content-Type", "Application/json"} }, body);
		if (id > 0)
		{
			printf("send req %lld ok, url =%s \n", id, url.c_str());
		}
		else
		{
			int err = httpClient.getLastErr();
			if (err == Error::Unreachable)
			{
				//N = i;
				printf("num = %lld, cant'connect host= %s:%d \n", id, ip.c_str(), port);
				//break;
			}
			else
			{
				printf("req %lld :send error %s \n", id, httpClient.getLastErrInfo().c_str());
			}

			{
				std::lock_guard<std::mutex> guard(mutexList);
				sendList.emplace_back(url);
			}

		}
		//Utils::sleepFor(1000);
	}

	printf("send finish here \n");
	while (!bExit)
	{
		Utils::sleepFor(10);
		if (count >= N)
		{
			double delta = timer.stop_delta_ms();
			printf("cost %f \n", delta);
			break;
		}
	}

	printf("recv finish here%d \n", count);

	httpClient.closeConnection();

	LibNet::EventLoopThreadPool::instance().stop();
}

void testSynGet()
{
	LibNet::EventLoopThreadPool::instance().Init(1).start();

	string ip = "220.181.38.251";
	ip = "10.128.6.236";
	int port = 80;
	HttpClient httpClient(ip, port);
	httpClient.setCmdTimeout(2);

	//httpClient.Get("/1.html");

	Timer  timer;
	int count = 0;
	int N = 300;
	for (int i = 0; i < N; i++)
	{
		char path[260];
		snprintf(path, 260, "/test.php?id=%d", i + 1);
		int64_t id = httpClient.Get(path);
		if (id > 0)
		{
			printf("id = %lld, code = %d, body = {%s} \n",
				id, 
				httpClient.getResponse()->code,
				httpClient.getResponse()->body.c_str());
		}
		else
		{
			int err = httpClient.getLastErr();
			if (err == Error::Unreachable)
			{
				N = i;
				printf("id = %d cant'connect host= %s:%d, exit here \n",
					i+1, ip.c_str(), port);
				break;
			}
			else if (err == Error::Timeout)
			{
				printf("id = %d timeout\n", i + 1);
			}
			else
			{
				printf("id = %d err = %d\n", i + 1, err);
			}
		}
	}

	httpClient.closeConnection();

	LibNet::EventLoopThreadPool::instance().stop();
}

void testTimer()
{
	LibNet::EventLoopThreadPool::instance().Init(1).start();
	bExit = false;

	Timer  timer;
	timer.start();
	int N = 5;
	int count = 0;
	IOEventLoop* loop= LibNet::EventLoopThreadPool::instance().getNextLoop();
	int64_t id = loop->addTimer(1000, 5000, [&](int64_t id)
	{
		double delta = timer.stop_delta_ms();
		printf("cost %f \n", delta);
		count++;
		timer.start();
	});

	while (!bExit)
	{
		Utils::sleepFor(10);
		if (count >= N)
		{
			loop->delTimer(id);
			break;
		}
	}
	LibNet::EventLoopThreadPool::instance().stop();
}


void testAsyn() 
{
	// must init thread pool first;
	LibNet::EventLoopThreadPool::instance().Init(1).start();

	string ip = "220.181.38.251";
	ip = "10.128.6.236";
	int port = 80;
	HttpClient httpClient(ip, port);
	httpClient.setCmdTimeout(2);

	int count = 0;
	int N = 30;

	HttpEventCallbackPipe cb =
		[&](std::shared_ptr<Request> &req, ResponsePtr & res)
	{

		if (res->error == Error::Success)
		{
			count++;	
			printf("===>code = %d, body = %s \n", res->code, res->body.c_str());
		}
		else if (res->error == Error::ConnectionReset)
		{
			printf("---> reset url = %s\n", req->path.c_str());
			
		}
		else if (res->error == Error::Timeout)
		{
			printf("---> time out url = %s\n", req->path.c_str());
		}
		else
		{
			printf("---> error = %d\n", res->error);
		}

		int64_t id = 0;
		do 
		{
			char path[260];
			snprintf(path, 260, "/test.php?id=%d", count + 1);
			sendList.emplace_back(path);
			id = httpClient.PostAsyn(path, Headers{ {"Content-Type", "Application/json"} }, body);
			if (id < 1)
			{
				int err = httpClient.getLastErr();
				printf("send id = %d err = %d\n", count + 1, err);
			}
			else
			{
				printf("send id = %d\n", count + 1);
			}
		} while (id < 1);
		

	};
	httpClient.enableAsynCmd(cb);

	char path[260];
	snprintf(path, 260, "/test.php?id=%d", count + 1);
	sendList.emplace_back(path);
	int64_t id = httpClient.PostAsyn(path, Headers{ {"Content-Type", "Application/json"} }, body);
	if (id < 1)
	{
		int err = httpClient.getLastErr();
		printf("id = %d err = %d\n", 1, err);
		return;
	}
	else
	{
		printf("send id = %d\n", count + 1);
	}

	while (count < N)
	{
		Utils::sleepFor(10);
	}

	printf("recv finish here%d \n", count);

	httpClient.closeConnection();

	LibNet::EventLoopThreadPool::instance().stop();
}

int main(int argc, char *argv[])
{
	//testPipelining();

	//testSynGet();
	//testTimer();

	testAsyn();
	return 0;
}



