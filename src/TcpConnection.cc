#include "../include/TcpConnection.h"
#include "../include/EventLoopThreadPool.h"
#include "../include/IOEventLoop.h"
#include "../include/Socket.h"
#include "../include/CommonLog.h"
#include "../include/CommonHeader.h"
#include <memory>
#include <future>

#ifdef WITH_OPENSSL
#include "../include/SocketSSL.h"
#endif

using namespace LibNet;

void defaultConnectionCallback(const TcpConnectionPtr &conn)
{
	std::stringstream ss;
	ss << "connnect return : " << conn->getLocalAddress().toIpPort() << " ----> "
		<< conn->getPeerAddress().toIpPort() << " is "
		<< (conn->isConnected() ? "UP" : "DOWN") << endl;
	FORMAT_DEBUG(ss.str().c_str());
	
}

void defaultMessageCallback(const TcpConnectionPtr &, Buffer *buf, Timestamp)
{
	//buf->retrieveAll();
}

////////////////////////////////////////////////////////////////////////////

TcpConnection::TcpConnection(const string &ip, int port) :
	remoteIP(ip), remotePort(port), localLoop(nullptr), currentStat(kDisconnected), 
	readBuffer(20), closeCallbackSynWait(nullptr), bUseSSL(false)
{
	this->peerAddr = InetAddress(remoteIP, remotePort, false);
}

TcpConnection::~TcpConnection()
{
	clearCallBack();
	DEBUG_PRINT("~TcpConnection()   \n");
}

void TcpConnection::initSocket()
{
	assert(localLoop);
#ifdef WITH_OPENSSL
	if (bUseSSL)
	{
		if (nullptr == localSocket)
		{
			FORMAT_DEBUG("defined WITH_OPENSSL, use ssl is true");
			localSocket = std::make_shared<SocketSSL>(localLoop, INVALID_SOCKET);
		}
	}
	else
	{
		if (nullptr == localSocket)
		{
			FORMAT_DEBUG("defined WITH_OPENSSL, use ssl is false");
			localSocket = std::make_shared<Socket>(localLoop, INVALID_SOCKET);
		}
	}
#endif

	if (nullptr == localSocket)
	{
		FORMAT_DEBUG("not defined WITH_OPENSSL, normal socket");
		localSocket = std::make_shared<Socket>(localLoop, INVALID_SOCKET);
	}
	
}

void TcpConnection::setSecurityMode()
{
#ifdef WITH_OPENSSL
	bUseSSL = true;
#endif
}

bool TcpConnection::clearCallBack()
{
	if (localSocket)
	{
		localSocket->setCloseCallback(nullptr);
		localSocket->setReadCallback(nullptr);
		localSocket->setWriteCallback(nullptr);
		localSocket->setErrorCallback(nullptr);
	}

	return true;
}

void TcpConnection::setClientCallback(BeforeReadCallback cliAlloc, AfterReadCallback cliRead,
	AfterWriteCallback cliWrite, AfterCloseCallback cliClose)
{
	clientAllocCallback = cliAlloc;
	clientReadCallback = cliRead;
	clientWriteCallback = cliWrite;
	clientCloseCallback = cliClose;
}

// must't use block with promise 
// when user call this function in callback,
// function in the same loop will never find return
bool TcpConnection::connectSyn(int secs)
{
	// should try again
	if (kDisconnecting == currentStat)
	{
		FORMAT_DEBUG("connectSyn() meets state == kDisconnecting\n");
		return false;
	}

	// no need
	if (kConnected == currentStat)
	{
		FORMAT_DEBUG("connectSyn() meets state == kDisconnected\n");
		return true;
	}

	// try again 
	if (kConnecting == currentStat)
	{
		FORMAT_DEBUG("connectSyn() meets state == kConnecting\n");
		return false;
	}

	if (this->localLoop == nullptr)
	{
		this->localLoop = EventLoopThreadPool::instance().getNextLoop();
		if (this->localLoop == nullptr)
			return false; // not run thread
	}
	// create socket as needed
	initSocket();

	// set socket to block mode
	int ret = localSocket->connect(peerAddr, true);
	if (ret != 0)
	{
		this->lastError = localSocket->getLastErr();
		this->errInfo = localSocket->getErrorInfo();
		setState(kDisconnected);
		return false;
	}

	setState(kConnected);

	// set non-block mode
	localSocket->setBlockMode(false);
	connecting();

	return true;
}

// used by client
bool TcpConnection::connect(int secs, ConnectionEventCallback cb)
{
	if (kConnecting == currentStat)
		return true;

	if (kConnected == currentStat)
		return true;

	if (kDisconnecting == currentStat)
		return false;

	setState(kConnecting);

	this->clientConnCallback = cb;

	if (this->localLoop == nullptr)
	{
		this->localLoop = EventLoopThreadPool::instance().getNextLoop();
		if (this->localLoop == nullptr)
			return false; // not run thread
	}
	if (nullptr == this->localSocket)
		this->localSocket = std::make_shared<Socket>(localLoop, INVALID_SOCKET);

	if (secs > 0)
	{
		localSocket->setSendTimeout(secs);
	}

	int ret = localSocket->connect(peerAddr);

	this->lastError = (ret == 0) ? 0 : localSocket->getLastErr();
	
	FORMAT_DEBUG("TcpConnection::connect() connect() error =%d ", lastError);
	SOCK_STATUS code = localSocket->waitOrClose();
	if (code == SOCK_STATUS::SOCK_OK || code == SOCK_STATUS::SOCK_WAIT)
	{
		connecting();
		return true;
	}
	this->errInfo = localSocket->getErrorInfo();
	FORMAT_ERROR("connect error=%d, %s", this->lastError, errInfo.c_str());
	setState(kDisconnected);
	return false;
}

void TcpConnection::connecting()
{
	// prevent that connection is deleted before socket
	localSocket->tie(this->shared_from_this());
	localSocket->setWriteCallback(std::bind(&TcpConnection::onWrite, this));
	localSocket->setErrorCallback(std::bind(&TcpConnection::onError, this));
	localSocket->setCloseCallback(std::bind(&TcpConnection::onClose, this));
	localSocket->setReadCallback(std::bind(&TcpConnection::onRead, this, std::placeholders::_1));

	// usually, client should send hello, so test writable is ok!
	// so, here wait int for connect ok or not

	this->startReadWrite();
}

//////////////////////////////////////////////////////////////////
void TcpConnection::onRead(Timestamp t)
{
	assert(kConnected == currentStat);
	if (kConnected == currentStat)
	{
		handleReadEvent(t);
	}
}

void TcpConnection::onWrite()
{
	if (kConnecting == currentStat)
	{
		handleConnectEvent();
	}
	else if (kConnected == currentStat)
	{
		handleWriteEvent();
	}
	else
	{
		// fatal error!!
	}
}

void TcpConnection::onError()
{
	if (kConnecting == currentStat)
	{
		// windows, getlasterror is 0
		setState(kDisconnected);
		if (clientConnCallback)
			clientConnCallback(this->shared_from_this());

	}
	else
	{
		handleCloseEvent();
	}

}

void TcpConnection::onClose()
{
	handleCloseEvent();
}
////////////////////////////////////////////////////////////////

// for thread safe, cmd must run in loop thread, or will crash

void TcpConnection::startReadWrite()
{
	assert(localLoop);

	localLoop->runInLoop(localSocket.get(), [=]()
	{
        // notice here: read is ok,set write when write fail
        //localSocket->enableAll();
        localSocket->enableReading();
	});
}

void TcpConnection::startWrite()
{
	if (!localLoop)
		return;

	localLoop->runInLoop(localSocket.get(), [=]()
	{
		localSocket->enableWriting();
	});
}

void TcpConnection::stopWrite()
{
	if (!localLoop)
		return;

	localLoop->runInLoop(localSocket.get(), [=]()
	{ localSocket->disableWriting(); });
}

void TcpConnection::startRead()
{

	if (!localLoop)
		return;

	localLoop->runInLoop(localSocket.get(), [=]()
	{ localSocket->enableReading(); });
}

void TcpConnection::stopRead()
{
	if (!localLoop)
		return;

	localLoop->runInLoop(localSocket.get(), [=]()
	{ localSocket->disableReading(); });
}


//called by client
bool TcpConnection::close()
{
	if (!localLoop)
		return true;   // never inited,

	if (kDisconnected == currentStat)
		return true;

	if (kDisconnecting == currentStat)
		return false;

	//assert(currentStat == kConnected);

	currentStat = kDisconnecting;

	localLoop->runInLoop(localSocket.get(), [=]()
	{
		onClose();
	});

	return false;
}

//called by client
bool TcpConnection::closeSyn()
{
	if (kDisconnected == currentStat)
		return true;

	if (kDisconnecting == currentStat)
		return false;

	//assert(currentStat == kConnected);

	currentStat = kDisconnecting;


	std::promise<bool> promisePtr;
	std::future<bool> futurePtr = promisePtr.get_future();

	// save old, block with promise

	this->closeCallbackSynWait = [&](const TcpConnectionPtr &)
	{
		try
		{
			promisePtr.set_value(true);
			// restore		
		}
		catch (std::exception& ex)
		{
			std::cout << "wait close():"<< ex.what() << endl;
		}
		
	};

	localLoop->runInLoop(localSocket.get(), [=]()
	{
		onClose();
	});

	FORMAT_DEBUG("TcpConnection::closeSyn() syn wait for close");
	bool ret = futurePtr.get();
	this->closeCallbackSynWait = nullptr;
	FORMAT_DEBUG("TcpConnection::closeSyn() syn wait for close return! setState(kDisconnected)" );

	currentStat = kDisconnected;

	return ret;
}
/////////////////////////////////////////////////////////////////////////

//bool TcpConnection::getTcpInfo(struct tcp_info *tcpi) const
//{
//  return localSocket->getTcpInfo(tcpi);
//}

//string TcpConnection::getTcpInfoString() const
//{
//  char buf[1024];
//  buf[0] = '\0';
//  localSocket->getTcpInfoString(buf, sizeof buf);
//  return buf;
//}
void TcpConnection::setTcpNoDelay(bool on)
{
	localSocket->setTcpNoDelay(on);
}

const char *TcpConnection::stateToString() const
{
	switch (currentStat)
	{
	case kDisconnected:
		return "kDisconnected";
	case kConnecting:
		return "kConnecting";
	case kConnected:
		return "kConnected";
	case kDisconnecting:
		return "kDisconnecting";
	default:
		return "unknown state";
	}
}

////////////////////////////////////////////////////////
//
bool  TcpConnection::send(const char *data, int len, void *pVoid)
{
	if (kConnected == currentStat)
	{
		if (localLoop)
		{
			localLoop->runInLoop(localSocket.get(), [=]()
			{
				// when lamda is running, must test socket fd is the old connected socecket or not;
				this->realSend(data, len, pVoid);			
			});
			return true;
		}
	}
	else
	{
		lastError = 0;
		errInfo = "socket is not connected";
		FORMAT_ERROR(errInfo.c_str());
	}

	return false;
}

// user write &&  run in loop
void TcpConnection::realSend(const char *data, size_t len, void *pVoid)
{
	ssize_t nwrote = 0;
	size_t remaining = len;
	bool faultError = false;
	if (currentStat == kDisconnected)
	{
		FORMAT_ERROR("disconnected, give up writing");
		return;
	}

	// if no thing in output queue, try writing directly
	if (!localSocket->isWriting() && outputQue.size() == 0)
	{
		nwrote = localSocket->write(data, len);
		if (nwrote >= 0)
		{
			remaining = len - nwrote;
			if (remaining == 0 && clientWriteCallback)
			{
				// write finish
				clientWriteCallback(this->shared_from_this(), data, len, pVoid, 0);
			}
		}
		else // nwrote < 0
		{
			nwrote = 0;
			this->lastError = localSocket->getLastErr();
			this->errInfo = localSocket->getErrorInfo();
			if (localSocket->waitOrClose() == SOCK_STATUS::SOCK_ERROR)
			{
				FORMAT_DEBUG("TcpConnection::realSend() run loop error= %d, %s", 
					lastError, this->errInfo.c_str());

				handleCloseEvent();

			}
		}
	}

	assert(remaining <= len);
	if (!faultError && remaining > 0)
	{
		this->outputQue.emplace_back(const_cast<char *>(data), len, len - remaining, pVoid);
		if (!localSocket->isWriting())
		{
			localSocket->enableWriting();
		}
	}
}

/////////////////////////////////////////////////////////////////
void TcpConnection::handleConnectEvent()
{
	lastError = localSocket->getLastErr();
	if (lastError)
	{
		FORMAT_DEBUG("connect error=%s setState(kDisconnected)", localSocket->getErrorInfo().c_str());
		setState(kDisconnected);
		if (clientConnCallback)
		{
			clientConnCallback(this->shared_from_this());
		}
	}
	else
	{
		FORMAT_DEBUG("setState(kConnected)");
		setState(kConnected);
		defaultConnectionCallback(this->shared_from_this());
		if (clientConnCallback)
		{
			clientConnCallback(this->shared_from_this());
		}
		else
		{
			localSocket->close();
			FORMAT_DEBUG("there is no Connect callback, localSocket->close(), setState(kDisconnected)");
			setState(kDisconnected);
		}
	}
}

void TcpConnection::handleWriteEvent()
{
	if (outputQue.size() == 0)
	{
		return;
	}

	if (localSocket->isWriting())
	{
		while (outputQue.size() > 0)
		{
			char *data = outputQue[0].data + outputQue[0].begin;
			size_t len = outputQue[0].len - outputQue[0].begin;
			ssize_t n = localSocket->write(data, len);
			if (n > 0)
			{
				if (n < len)
				{
					outputQue[0].begin += n;
					break;
				}
				else
				{
					void *param = outputQue[0].param;
					outputQue.pop_front();
					if (clientWriteCallback)
					{
						clientWriteCallback(this->shared_from_this(), data, len, param, 0);
					}
					continue;
				}
			}
			else  if (n == 0)// n==0, close;  -1 will block  must do signal(SIGPIPE, SIG_ING), or will exit
			{
				handleCloseEvent();
			}
			else // -1 and other
			{
				this->lastError = localSocket->getLastErr();
				this->errInfo = localSocket->getErrorInfo();
				
				SOCK_STATUS code = localSocket->waitOrClose();
				if (code == SOCK_STATUS::SOCK_ERROR)
				{
					FORMAT_DEBUG("write return -1, last error = %d, %s", this->lastError, this->errInfo.c_str());
					handleCloseEvent();
				}
				else    // would block
				{
					break;
				}
			} // -1
	
		}// end while
	}
	else
	{
		std::stringstream ss;
		ss << "Connection fd = " << localSocket->fd() << "is down, no more writing ";
		FORMAT_ERROR(ss.str().c_str());
		handleCloseEvent();
	}
}

void TcpConnection::handleReadEvent(Timestamp receiveTime)
{
	int savedErrno = 0;
	char *data = nullptr;
	size_t len = 0;
	void *param = 0;
	ssize_t n = 0;

	do
	{
		if (clientAllocCallback)
			clientAllocCallback(this->shared_from_this(), &data, &len, &param);

		if ((data == nullptr) || (len == 0))
		{
			readBuffer.ensureWritableBytes(4096);
			readBuffer.clear();
			data = (char *)readBuffer.peek();
			len = readBuffer.writableBytes();
			param = &readBuffer;
		}

		n = localSocket->read(data, len);

		if (n == 0)
		{
			FORMAT_INFO("TcpConnection::handleReadEvent() read return 0, remote close socket");
			handleCloseEvent();
			break;
		}
		else if (n > 0)
		{
			FORMAT_DEBUG("TcpConnection::handleReadEvent() read return %zu", n);
			//printf("read %zu\n", n);
			if (clientReadCallback)
				clientReadCallback(this->shared_from_this(), data, n, param);
		}
		else
		{
			this->lastError = localSocket->getLastErr();
			this->errInfo = localSocket->getErrorInfo();

			SOCK_STATUS ret = localSocket->waitOrClose();
			if (ret == SOCK_STATUS::SOCK_ERROR)  // close
			{
				FORMAT_DEBUG("read return -1, last error = %d, %s", this->lastError, this->errInfo.c_str());
				handleCloseEvent();		
			}
			break;
		}


	} while (n == static_cast<ssize_t>(len));


}

// lastError is setted before this function
void TcpConnection::handleCloseEvent()
{
	setState(kDisconnecting);

	localSocket->remove();
	localSocket->close();

	// clean send queue
	TcpConnectionPtr guardThis(shared_from_this());
	while (outputQue.size() > 0)
	{
		if (clientWriteCallback)
		{
			clientWriteCallback(guardThis, outputQue[0].data, 0, outputQue[0].param, this->lastError);
		}
		outputQue.pop_front();
	}
	// must set state before call user callback, 
	// user should call new Connection in callback
	FORMAT_DEBUG("TcpConnection::handleCloseEvent() setState= kDisconnected");
	setState(kDisconnected);

	// synClose wait
	if (closeCallbackSynWait)
	{
		FORMAT_DEBUG("TcpConnection::closeCallbackSynWait() ...");
		closeCallbackSynWait(guardThis);
	}
	else
	{
		// normal client close callback
		FORMAT_DEBUG("TcpConnection::clientCloseCallback() ...");
		if (clientCloseCallback)
			clientCloseCallback(guardThis);
	}

	
	
}

int  TcpConnection::getLastErr()
{
	return this->lastError;
}
string  TcpConnection::getLastErrInfo()
{
	return this->errInfo;
}

IOEventLoop* TcpConnection::getLoop()
{
	return this->localLoop;
}

