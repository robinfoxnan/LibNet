#pragma  once
#include <future>

#include "../include/EventLoopThreadPool.h"
#include "../include/IOEventLoop.h"
#include "../include/TcpConnection.h"
#include "../include/Buffer.h"
#include "./Detail.h"
#include "./http_parser.h"

namespace LibNet
{
	const int64_t  HTTP_CHECK_SPEED = 2000;
	/*class Response;
	class Headers;
	
	using ResponsePtr = std::shared_ptr<Response>;*/
	
	using ms = std::ratio<1, 1000>;

	class HttpClient;
	using HttpEventCallback = function<void(ResponsePtr &)>;
	using HttpEventCallbackPipe = function<void(HttpClient*, std::shared_ptr<Request> &, ResponsePtr &)>;
	enum ClientState
	{
		SENDING = 1,
		WAITING_CODE,
		WAITING_HEADER,
		WAITING_BODY,
		RES_OK
	};

	enum HttpClientMode
	{
		IDLE = 0,
		SYN,
		ASYN,
		PIPELINING
	};
	
	class HttpClient
	{
	public:
		HttpClient(const std::string &host, int port);
		~HttpClient();

		Error  getLastErr();
		string  getLastErrInfo();

		void setConnectTimeout(int t);    // in seconds, must set before working, or timer can't get it
		void setCmdTimeout(int t);        // in seconds, must set before working, or timer can't get it
		void enableAsynCmd(HttpEventCallbackPipe& callback);
		void setHttpsMode();

		int64_t Get(const string & path);
		int64_t Get(const string & path, const Headers &headers);
		int64_t GetAsyn(const string & path, const Headers &headers);
		int64_t GetInPipeline(const string & path, const Headers &headers);
		
		void enablePipeline(HttpEventCallbackPipe& callback);

		int64_t Post(const string & path);
		int64_t Post(const string & path, const Headers &headers);
		int64_t Post(const string & path, const Headers &headers, const string &body,
			const string &contentType = "text/html");

		int64_t PostAsyn(const string & path, const Headers &headers, const string &body,
			const string &contentType = "text/html");
		int64_t PostInPipeline(const string & path, const Headers &headers, const string & body);

		ResponsePtr getResponse();

		bool closeConnection();
		void onTimer(int64_t id);

		size_t getQueLen();
		size_t getRpcSpeed();

	private:
		int64_t CmdInPipeline(const string & path, const Headers &headers, const char * cmd, const string & body);
		int64_t  sendRequestInPipeline(std::shared_ptr<Request> &req, bool isLine = true);
		void notifyUserError();

		int64_t  sendRequestSyn(const string &body, const string &contentType);
		// parse response;
		bool parseResponse();
		bool makeConnecion();
		

		// connection callback
		void onConnectionClose(const TcpConnectionPtr& connection);
		void onConnectReturn(const TcpConnectionPtr& conn);
		void onWriteEnd(const TcpConnectionPtr &conn, const char * data, size_t sz, void *pVoid, int status);
		void onAllocBuffer(const TcpConnectionPtr &conn, char * *data, size_t *sz, void **pVoid);
		void onReadData(const TcpConnectionPtr &conn, char * data, size_t sz, void *pVoid);
		void testSpeed();
		// timer related
		void startTimer();
		void stopTimer();

	public:

		// http_parser callback
		void reset(bool isPipeLine = false);
		void onFieldName(const char * buf, size_t len);
		void onFieldValue(const char * buf, size_t len);
		void onBodyData(const char * buf, size_t len);
		void onHeaderEnd(int code);
		void onResEnd(int err);
	

	private:
		Error  curError;
		string curErrInfo;
		HttpClientMode workMode;   // working in three mode

		http_parser parser;     // use node.js http_parser

		std::string host_name;
		int host_port;

		int timeoutConnect;    // timeout seconds
		int timeoutCmd;

		Buffer localBuffer;
		std::mutex  mutexBuf;
		bool bHttps;

		bool bKeepAlive;
		bool bEncodeUrl;

		TcpConnectionPtr  conn;

		using RequestPtr = std::shared_ptr<Request> ;

		std::mutex mutextTimer;
		int64_t timerId;

		ClientState currentStat;
		RequestPtr currentReq;           // http work mode is: send -->> recv , send -->> recv

		ResponsePtr  currentRes;
		string       currentFieldName;   // parser use it 


		// syn Get/POST
		std::promise<int> currentWait;   // worker thread  .set_val(), so we block and get()

		HttpEventCallback responseCallback;     // recv one response finished, call it 
		HttpEventCallbackPipe pipeResCallback;

		// http1.1 send request in pipeline,
		int64_t seqId;
		std::mutex seqMutex;
		std::deque<std::shared_ptr<Request> > requestQue;

		// used to calculate speed
		int64_t recvCount;   // count recv response, and calculate rpc speed
		int64_t lastCount;
		double  rpcSpeed;
		std::chrono::time_point<std::chrono::high_resolution_clock> tmLast;
	};
}

