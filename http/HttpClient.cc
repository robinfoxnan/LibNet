#include "../include/CommonHeader.h"
#include "../include/Util.h"
#include "../include/CommonLog.h"

#include "HttpClient.h"
#include "../include/windows/TcpConnectionIocp.h"



using namespace LibNet;
using namespace std::placeholders;
//////////////////////////////////////////////////////////////////////////////////////////
int message_begin_cb(http_parser* p)
{
	return 0;
}


int header_field_cb(http_parser* p, const char* buf, size_t len)
{
	//string field(buf, len);
	//DEBUG_PRINT("%s", field.c_str());

	if (p->data)
	{
		static_cast<HttpClient *>(p->data)->onFieldName(buf, len);
	}

	return 0;
}

int header_value_cb(http_parser* p, const char* buf, size_t len)
{
	//string field(buf, len);
	//DEBUG_PRINT(":%s\n", field.c_str());
	if (p->data)
	{
		static_cast<HttpClient *>(p->data)->onFieldValue(buf, len);
	}
	return 0;
}

int request_url_cb(http_parser* p, const char* buf, size_t len)
{
	//string field(buf, len);
	//DEBUG_PRINT("url = %s\n", field.c_str());
	return 0;
}

int response_status_cb(http_parser*p, const char* buf, size_t len)
{
	//string field(buf, len);
	//DEBUG_PRINT("status=%s\n", field.c_str());
	return 0;
}


int body_cb(http_parser* p, const char* buf, size_t len)
{
	string body(buf, len);
	DEBUG_PRINT("body = %s \n", body.c_str());
	
	if (p->data)
	{
		static_cast<HttpClient *>(p->data)->onBodyData(buf, len);
	}
	return 0;
}

int headers_complete_cb(http_parser* p)
{
	int code = p->status_code;
	DEBUG_PRINT("code=%d\n", code);

	if (p->data)
	{
		static_cast<HttpClient *>(p->data)->onHeaderEnd(code);
	}
	return 0;
}

int message_complete_cb(http_parser* p)
{
	DEBUG_PRINT("---------------------------\n");
	if (p->data)
	{
		static_cast<HttpClient *>(p->data)->onResEnd(p->http_errno);
	}
	return 0;
}

int chunk_header_cb(http_parser* p)
{
	return 0;
}

int chunk_complete_cb(http_parser* p)
{
	return 0;
}

/////////////////////////////////////////////////////////////////
http_parser_settings parser_settings = { 
	message_begin_cb, 
	request_url_cb,
	response_status_cb, 
	header_field_cb, 
	header_value_cb,
	headers_complete_cb, 
	body_cb, 
	message_complete_cb, 
	chunk_header_cb, 
	chunk_complete_cb 
};

void HttpClient::onFieldName(const char * buf, size_t len)
{
	currentFieldName.assign(buf, len);
}
void HttpClient::onFieldValue(const char * buf, size_t len)
{
	if (!currentFieldName.empty())
	{
		string val(buf, len);
		currentRes->headers[currentFieldName] = val;
		currentFieldName = "";
	}
}
void HttpClient::onBodyData(const char * buf, size_t len)
{
	currentRes->body.append(buf, len);
}
void HttpClient::onHeaderEnd(int code)
{
	currentRes->code = code;
}

// http_parser call here, end one response
void HttpClient::onResEnd(int err)
{
	this->currentRes->error = Error::Success;
	this->testSpeed();

	if (this->responseCallback)
	{
		// pipeling mode , call lamda in 
		// call pipeResCallback inside!
		this->responseCallback(currentRes);

	}
	// finish one resonse parse	
	// reset response with mode pipelining & asyn
	if (this->pipeResCallback)
	{
		this->reset(true);
	}
	else
	{
		this->reset(false);
	}
}

// call this function when recv a response
void HttpClient::testSpeed()
{
	recvCount++;
	if ((recvCount - lastCount) > HTTP_CHECK_SPEED)
	{
		std::chrono::time_point<std::chrono::high_resolution_clock> tpStop = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, ms> tmDelta = std::chrono::duration<double, ms>(tpStop - tmLast);
		double delta = tmDelta.count();
		if (delta > 0)
		{
			this->rpcSpeed = 1000.0 * (recvCount - lastCount) / delta;

			// reset
			lastCount = recvCount;
			tmLast = std::chrono::high_resolution_clock::now();
		}
	}
}

void HttpClient::startTimer()
{
	std::lock_guard<std::mutex> guard(mutextTimer);
	if (conn == nullptr)
	{
		FORMAT_ERROR("startTimer(): try to start timer, but conn is nullptr ");
	}
	else
	{
		if (this->conn && (this->timerId < 1) )
		{
			this->timerId = conn->getLoop()->addTimer(timeoutCmd * 1000,
				1000,
				std::bind(&HttpClient::onTimer, this, _1)
			);
		}
	}
}

void HttpClient::stopTimer()
{
	std::lock_guard<std::mutex> guard(mutextTimer);
	if (conn == nullptr)
	{
		FORMAT_ERROR("stopTimer(): try to start timer, but conn is nullptr ");
	}
	else
	{
		if (this->conn)
		{
			if (timerId > 0)
			{
				conn->getLoop()->delTimer(timerId);
				timerId = -1;
			}
			else
			{
				FORMAT_ERROR("stopTimer(): timerId == -1, return");
			}
		}
	}
}

// check pipeline and asy mode whether is ok
void HttpClient::onTimer(int64_t id)
{
	// lock domain
	bool isTimeout = false;
	{
		std::lock_guard<std::mutex> lock(seqMutex);
		if (requestQue.size() > 0)
		{
			int64_t tm = Utils::getmSecNow();
			int64_t delta = tm - requestQue[0]->startTm;
			if (delta > timeoutCmd * 1000)
			{
				// timeout			
				isTimeout = true;
			}
		}
	}

	if (isTimeout)
	{
		conn->closeSyn();
		this->currentRes->error = Error::Timeout;
		this->curError = Error::Timeout;
		notifyUserError();
	}
		

}

///////////////////////////////////////////////////////////////////////////////////////////
HttpClient::HttpClient(const std::string &host, int port) :host_name(host), host_port(port),
	bKeepAlive(true), conn(nullptr), timeoutCmd(3), timeoutConnect(5), responseCallback(nullptr), 
	pipeResCallback(nullptr), seqId(1), timerId(-1), workMode(HttpClientMode::IDLE),recvCount(0),
	rpcSpeed(0), tmLast{ std::chrono::high_resolution_clock::now() }, bHttps(false)
{
	
	if (currentRes == nullptr)
	{
		currentRes = std::make_shared<Response>();
		currentRes->reset();
	}

	if (currentReq == nullptr)
	{
		currentReq = LibNet::make_unique<Request>();
		currentReq->reset();
	}
	reset();
}

HttpClient::~HttpClient()
{
	if (conn)
	{ 
		stopTimer();
		conn->closeSyn();
		conn->setClientCallback(nullptr, nullptr, nullptr, nullptr);
	}

	if (currentRes)
		currentRes = nullptr;

	if (currentReq)
		currentReq = nullptr;
}

void HttpClient::reset(bool isPipeLine)
{
	if (isPipeLine)
		this->currentRes = std::make_shared<Response>();
	http_parser_init(&parser,  HTTP_RESPONSE);
	parser.data = this;
	currentFieldName = "";

	//localBuffer.state();
}

void HttpClient::setConnectTimeout(int t)
{
	if (t < 0)
		t = 0;
	timeoutConnect = t;
}

void HttpClient::setCmdTimeout(int t)
{
	if (t < 0)
		t = 0;
	timeoutCmd = t;
}
void HttpClient::setHttpsMode()
{
	bHttps = true;
}

bool HttpClient::closeConnection()
{
	if (conn)
	{
		//int count = conn.use_count();
		stopTimer();
		conn->closeSyn();
		conn->setClientCallback(nullptr, nullptr, nullptr, nullptr);
		//count = conn.use_count();
		conn.reset();
	}
	conn = nullptr;

	return true;
}

/////////////////////////////////////////////////////////////
// user interface with sym mode
ResponsePtr HttpClient::getResponse()
{
	return this->currentRes;
}

int64_t HttpClient::Get(const string & path)
{
	return Get(path, Headers());
}

int64_t HttpClient::Get(const string & path, const Headers &headers)
{
	this->currentReq->reset();
	this->currentReq->method = "GET";
	this->currentReq->path = path;
	this->currentReq->headers = headers;

	int64_t  ret = sendRequestSyn("", "");
	return ret;
}

int64_t HttpClient::Post(const string & path)
{
	return Post(path, Headers(), "", "text/html");
}

int64_t HttpClient::Post(const string & path, const Headers &headers)
{
	return Post(path, headers, "", "text/html");
}

int64_t HttpClient::Post(const string & path, const Headers &headers, const string &body, const string &contentType)
{
	this->currentReq->reset();
	this->currentReq->method = "POST";
	this->currentReq->path = path;
	this->currentReq->headers = headers;
	int64_t ret = sendRequestSyn(body, contentType);
	return ret;
}

int64_t  HttpClient::sendRequestSyn(const string &body, const string &contentType)
{
	workMode = HttpClientMode::SYN;

	this->currentRes->reset();
	this->currentWait = std::promise<int>();
	this->responseCallback = [=](ResponsePtr &)
	{
		try
		{
			this->currentWait.set_value(currentRes->error);
		}
		catch (std::exception& ex)
		{
			std::cout << ex.what() << endl;
		}
	};
	std::future<int> futurePtr = this->currentWait.get_future();

	// call send ansy
	int64_t id = sendRequestInPipeline(this->currentReq, false);
	if (id == 0)
		return 0;

	// block and get
	std::chrono::milliseconds delta(timeoutCmd * 1000);
	if (futurePtr.wait_for(delta) != std::future_status::ready)
	{

		FORMAT_DEBUG("sysn response return, timeout %d secs", timeoutCmd);
		conn->closeSyn();
		currentRes->error = Error::Timeout;
		this->curError = SetError(Error::Timeout, this->curErrInfo);

		return 0;
	}
	else
	{
		//DEBUG_PRINT("sysn response return, parse result=%d \n ", currentRes->error);
	}

	return id;
}

////////////////////////////////////////////////////////////////////////////
// user interface work mode asyn
// in same as pipelining
void HttpClient::enableAsynCmd(HttpEventCallbackPipe& callback)
{
	this->enablePipeline(std::forward<HttpEventCallbackPipe&>(callback));
}

int64_t HttpClient::GetAsyn(const string & path, const Headers &headers)
{
	workMode = HttpClientMode::ASYN;

	this->currentReq->reset();
	this->currentReq->method = "GET";
	this->currentReq->path = path;
	this->currentReq->headers = headers;

	return sendRequestInPipeline(this->currentReq, true);
}

int64_t HttpClient::PostAsyn(const string & path, const Headers &headers, const string &body,
	const string &contentType)
{
	workMode = HttpClientMode::ASYN;

	this->currentReq->reset();
	this->currentReq->method = "POST";
	this->currentReq->path = path;
	this->currentReq->headers = headers;

	return sendRequestInPipeline(this->currentReq, true);
}


///////////////////////////////////////////////////////////////////////////////
// user work in pipelining mode
// pipelining , one error will cause all requests fail
// asyn mode,only one in the que
void HttpClient::notifyUserError()
{
	std::deque<std::shared_ptr<Request> > tempQue;
	// swap all request to temp
	// lock domain
	{
		std::lock_guard<std::mutex> lock(seqMutex);
		requestQue.swap(tempQue);
	}

	// lock free here
	// there is a call back in the domain, so mustn't with a lock,
	// user may call a new request in callback, or use another lock, it's dangerous
	// clear que in mode pipelining and asyn
	while (tempQue.size() > 0)
	{
		if (pipeResCallback)
		{
			pipeResCallback(this, tempQue[0], currentRes);
		}

		tempQue.pop_front();
	}
	
}

void HttpClient::enablePipeline(HttpEventCallbackPipe &callback)
{
	this->pipeResCallback = callback;
	this->responseCallback = [=](ResponsePtr & res)
	{
		std::shared_ptr<Request> req = nullptr;
		{
			std::lock_guard<std::mutex> lock(seqMutex);
			assert(requestQue.size() > 0);
			req = requestQue[0];
			requestQue.pop_front();
		}
		
		// call user callback, without lock
		if (this->pipeResCallback)
		{
			this->pipeResCallback(this, req, res);
			
		}
	};

	this->currentRes = std::make_shared<Response>();
}

int64_t HttpClient::GetInPipeline(const string & path, const Headers &headers)
{
	return CmdInPipeline(path, headers, "GET", "");
}

int64_t HttpClient::PostInPipeline(const string & path, const Headers &headers, const string & body)
{
	return CmdInPipeline(path, headers, "POST", body);
}
int64_t HttpClient::CmdInPipeline(const string & path, const Headers &headers, const char * cmd, const string & body)
{
	std::shared_ptr<Request> req = std::make_shared<Request>();
	req->reset();
	req->method = cmd;
	req->path = path;
	req->headers = headers;
	req->body = body;
	
	return sendRequestInPipeline(req, true);
}
// it is a only function that all method used
int64_t HttpClient::sendRequestInPipeline(std::shared_ptr<Request> &req, bool isLine)
{
	FORMAT_DEBUG("HttpClient::sendRequestInPipeline begin to send %s", req->path.c_str());
	if (bKeepAlive)
		req->headers.emplace("Connection", "Keep-Alive");
	else	
		req->headers.emplace("Connection", "close");
	

	if (req->headers.find("Content-Type") == req->headers.end())
		req->headers.emplace("Content-Type", "text/html");

	
	

	req->headers.emplace("User-Agent", "HttpClient-0.9");
	req->headers.emplace("Host", host_name);

	if (!req->body.empty())
	{
		string lenStr = std::to_string(req->body.length());
		req->headers.emplace("Content-Length", lenStr);
	}

	stringstream ss;
	if (bEncodeUrl)
	{
		string path = req->path; //encode_url(req.path);
		ss << req->method << " " << path << " " << "HTTP/1.1\r\n";
	}
	else
	{
		ss << req->method << " " << req->path << " " << "HTTP/1.1\r\n";
	}

	// lock it 
	{
		std::lock_guard<std::mutex> lock(seqMutex);
		req->seqId = this->seqId++;
		req->headers.emplace("PIPELINE-ID", std::to_string(req->seqId));
	}


	for (auto it : req->headers)
	{
		ss << it.first << ":" << it.second << "\r\n";
	}
	ss << "\r\n";

	req->reqStr = ss.str();
	DEBUG_PRINT("begin to send header:\n");
	//DEBUG_PRINT("begin to send header:\n%s \n", req->reqStr.c_str());

	static int count = 0;

	// set timeout
	if (isLine)
	{
		startTimer();
	}

	if (makeConnecion())
	{
		FORMAT_DEBUG("makeConnecion() connect ok, begin to send :");
		std::lock_guard<std::mutex> lock(seqMutex);
		req->startTm = Utils::getmSecNow();
		bool ret = conn->send(req->reqStr.c_str(), (int)req->reqStr.length(), 0);
		if (ret)
		{
			if (!req->body.empty())
			{
				ret = conn->send(req->body.c_str(), (int)req->body.length(), 0);
				if (!ret)
				{
					FORMAT_DEBUG("send body error; will closeSyn()");
					if (conn)
						conn->closeSyn();
					// send header fail
					this->curError = SetError(Error::Write, curErrInfo);
					return 0;
				}
			}

			// work mode with asyn & pipelining
			if (isLine)
				requestQue.push_back(req);
			// or using currentReq instead
			/*else
				this->currentReq = req;*/
			return req->seqId;
		}
		else
		{
			FORMAT_DEBUG("send header error; will closeSyn()");
			if (conn)
				conn->closeSyn();

			// send header fail
			this->curError = SetError(Error::Write, curErrInfo);
			
		}
	}
	else
	{
		// connect fail
		FORMAT_DEBUG("makeConnecion() connect err, can't connect\n");
		this->curError = SetError(Error::Unreachable, curErrInfo);

	}

	

	return 0;
}

/////////////////////////////////////////////////////////////////
// common functions


bool HttpClient::makeConnecion()
{
	if (conn == nullptr)
	{
		FORMAT_DEBUG("HttpClient::makeConnecion() create new connection");

#if defined(_WIN32) || (_WIN64)
#if defined(WITH_IOCP)  // iocp mode
		conn = std::make_shared<TcpConnectionIocp>(this->host_name, host_port);
#else   // select mode
		conn = std::make_shared<TcpConnection>(this->host_name, host_port);
#endif
#else   // linux and mac
		conn = std::make_shared<TcpConnection>(this->host_name, host_port);
#endif

		if (bHttps)
			conn->setSecurityMode();
		// wait
		conn->setClientCallback(
			std::bind(&HttpClient::onAllocBuffer, this, _1, _2, _3, _4),
			std::bind(&HttpClient::onReadData, this, _1, _2, _3, _4),
			std::bind(&HttpClient::onWriteEnd, this, _1, _2, _3, _4, _5),
			std::bind(&HttpClient::onConnectionClose, this, _1)
		);
	}
	

	if (conn->isConnected())
	{
		FORMAT_DEBUG("HttpClient::makeConnecion() already connected");
		return true;
	}
	FORMAT_DEBUG("wait connectSyn...");
	bool ret = conn->connectSyn(this->timeoutConnect);
	//count = conn.use_count();
	FORMAT_DEBUG("wait connectSyn return ok=%d", (int)ret);
	return ret;
}



void HttpClient::onConnectionClose(const TcpConnectionPtr& connection)
{
	FORMAT_DEBUG("HttpClient::onConnectionClose() http connetion is closed!");

	this->currentRes->error = Error::ConnectionReset;
	this->curError = Error::ConnectionReset;

	// pipelining & asyn
	notifyUserError();
}

void HttpClient::onAllocBuffer(const TcpConnectionPtr &conn, char * *data, size_t *sz, void **pVoid)
{
	//DEBUG_PRINT("alloc buffer\n");
	//localBuffer.state();
	//*data = nullptr;
	//*sz = 0;
	//*pVoid = 0;

	if (localBuffer.writableBytes() < 2048)
	{
		localBuffer.addRoom(2048);
	}
	*data = (char *)this->localBuffer.peekEnd();
	*sz = this->localBuffer.writableBytes();
	*pVoid = &localBuffer;
}

// use http_parser to parse the data
void HttpClient::onReadData(const TcpConnectionPtr &conn, char * data, size_t sz, void *pVoid)
{
	assert(pVoid);
	
	//DEBUG_PRINT("HttpClient::onReadData:%zu, \n", sz);
	FORMAT_DEBUG("HttpClient::onReadData:%zu, \n %s \n", sz, data);
	//std::lock_guard<std::mutex> guard(mutexBuf);

	Buffer * buffer = static_cast<Buffer *>(pVoid);
	if (sz > 0 && buffer)
	{
		buffer->setAppend(sz);
		this->parseResponse();
		
		//buffer->state();
		buffer->checkToMove();
	}
}

bool HttpClient::parseResponse()
{
	size_t off = http_parser_execute(&parser, &parser_settings, localBuffer.peek(), localBuffer.readableBytes());
	if (off > 0)
	{
		localBuffer.retrieve(off);	
	}

	return true;
}

void HttpClient::onWriteEnd(const TcpConnectionPtr &conn, const char * data, size_t sz, void *pVoid, int status)
{
	string str(data, sz);
	FORMAT_DEBUG("HttpClient::onWriteEnd() send ok size = %zu, content=\n%s", sz, str.c_str());
	static int count = 0;
	
	//count++;
	//printf("send end count =%d\n\n", count);
}

void HttpClient::onConnectReturn(const TcpConnectionPtr& conn)
{
	DEBUG_PRINT("connect return: %s \n", (conn->isConnected()? "OK": "ERR"));

}
//////////////////////////////////////////////////////////////////////////////////////////
Error  HttpClient::getLastErr()
{
	return this->curError;
}
string  HttpClient::getLastErrInfo()
{
	return this->curErrInfo;
}

size_t HttpClient::getQueLen()
{
	std::lock_guard<std::mutex> lock(seqMutex);
	return requestQue.size();
}


size_t HttpClient::getRpcSpeed()
{
	return (size_t)rpcSpeed;
}

