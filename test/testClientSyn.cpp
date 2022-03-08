#include <iostream>
#include "../include/EventLoopThreadPool.h"
#include "../include/IOEventLoop.h"
#include "../include/TcpConnection.h"
#include "../include/Util.h"

#include "../http/HttpClient.h"

#include "../include/MyTimer.h"
#include "../include/CommonLog.h"

using namespace std;
using namespace  LibNet;


string body = R"({ "age": 5 })";



void testSynGet()
{
	LibNet::EventLoopThreadPool::instance().Init(1).start();

	string ip = "220.181.38.251";
    ip = "10.128.6.129";
    int port = 80;
	HttpClient httpClient(ip, port);
    // open ssl, open https://127.0.0.1:443/1.html
    //httpClient.setHttpsMode();
    httpClient.setCmdTimeout(5);

   // httpClient.Get("/1.html");

	Timer  timer;
	int count = 0;
	int N = 300;
	for (int i = 0; i < N; i++)
	{
		char path[260];
        snprintf(path, 260, "/test1.php?id=%d", i + 1);
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


int main(int argc, char *argv[])
{
	//testPipelining();

	testSynGet();
	//testTimer();

	//testAsyn();
	return 0;
}



