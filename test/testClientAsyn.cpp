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

void testAsyn() 
{
	// must init thread pool first;
	LibNet::EventLoopThreadPool::instance().Init(1).start();

	string ip = "220.181.38.251";
	ip = "127.0.0.1";
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
			// skip the timeout cmd
			count++;
		}
		else
		{
			printf("---> error = %d\n", res->error);
		}

		if (count >= N)
			return;

		int64_t id = 0;
		do 
		{
			char path[260];
			snprintf(path, 260, "/test.php?id=%d", count + 1);

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



