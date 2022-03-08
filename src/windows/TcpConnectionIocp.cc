#include "../../include/windows/TcpConnectionIocp.h"
#include "../../include/windows/SocketIocp.h"
#include "../../include/IOEventLoop.h"
#include "../../include/CommonLog.h"
#ifdef WITH_OPENSSL
#include "../../include/windows/SocketSSLIocp.h"
#endif

using namespace LibNet;

TcpConnectionIocp::TcpConnectionIocp(const string & ip, int port) : TcpConnection(ip, port)
{

}
TcpConnectionIocp::~TcpConnectionIocp()
{

}

void TcpConnectionIocp::initSocket()
{
	assert(localLoop);
#ifdef WITH_OPENSSL
	if (!bUseSSL)
	{
		if (nullptr == localSocket)
			localSocket = std::make_shared<SocketIocp>(localLoop, INVALID_SOCKET);
	}
	else
	{
		if (nullptr == localSocket)
			localSocket = std::make_shared<SocketSSLIocp>(localLoop, INVALID_SOCKET);
	}
#endif

	if (nullptr == localSocket)
		localSocket = std::make_shared<SocketIocp>(localLoop, INVALID_SOCKET);
}

void TcpConnectionIocp::startReadWrite()
{
	assert(localLoop);
	// thread safe in IocpPoller, so no need in the loop
	localSocket->enableWriting();
	this->preRead();
	
}

void TcpConnectionIocp::preRead()
{
	if (kConnected != currentStat)
	{
		FORMAT_ERROR("preRead() current state is not kconnected");
		return;
	}

	char *data = nullptr;
	size_t len = 0;
	void *param = 0;

	if (clientAllocCallback)
	{
		std::shared_ptr<TcpConnection> ptr = this->shared_from_this();
		clientAllocCallback(ptr, &data, &len, &param);
	}

	if ((data == nullptr) || (len == 0))
	{
		readBuffer.ensureWritableBytes(4096);
		readBuffer.clear();
		data = (char *)readBuffer.peek();
		len = readBuffer.writableBytes();
		param = &readBuffer;
	}

	ssize_t n = localSocket->read(data, len, param);
	if (n > 0)          // recv ok
	{
		//if (clientReadCallback)
		//{
		//	clientReadCallback(shared_from_this(), data, n, param);
		//}
		return;
	}
	else if (n == 0)   // io pending
	{
		return;
	}
	else
	{
		// error
		lastError = localSocket->getLastErr();
		errInfo = localSocket->getErrorInfo();
		FORMAT_DEBUG("TcpConnectionIocp::preRead() error= %d, %s",
			lastError, this->errInfo.c_str());

		handleCloseEvent();
	}
}


bool TcpConnectionIocp::send(const char* data, int len, void *pVoid)
{
	if (kConnected == currentStat)
	{
		ssize_t n = localSocket->write(data, len, pVoid);
		if (n >= 0)      // send finished, or pending
		{
			return true;
		}
		else
		{
			// error
			lastError = localSocket->getLastErr();
			errInfo = localSocket->getErrorInfo();
			FORMAT_DEBUG("TcpConnectionIocp::send() error= %d, %s",
				lastError, this->errInfo.c_str());

			// notice user there is something wrong
			if (clientWriteCallback)
				clientWriteCallback(shared_from_this(), (char *)data, 0, pVoid, -1);

			handleCloseEvent();
			return false;
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

//////////////////////////////////////////////////////////////////////////
void TcpConnectionIocp::handleConnectEvent()
{
	TcpConnection::handleConnectEvent();

	// if asyn connect result is ok 
	if (kConnected == currentStat)
		preRead();
}

void TcpConnectionIocp::handleWriteEvent()
{
	char * buf = nullptr;;
	void * param = nullptr;

	ssize_t len = localSocket ->writeEnd(&buf, &param);
	
	if (clientWriteCallback)
	{
		clientWriteCallback(this->shared_from_this(), buf, len, param, 0);
	}

	// if iocp write len < buf len, we trait it as error
	if (len <= 0)
	{
		FORMAT_ERROR("TcpConnectionIocp::handleWriteEvent() find that wsa send return ioSize < bufSize, so we close socket");
		handleCloseEvent();
	}
}

void TcpConnectionIocp::handleReadEvent(Timestamp receiveTime)
{
	char * buf;
	void * param = nullptr;

	ssize_t nRead = localSocket->readEnd(&buf, &param);

	// whatever, let user to know to free buffer
	if (clientReadCallback)
	{
		clientReadCallback(shared_from_this(), buf, nRead, param);
	}

	
	if (nRead <= 0)
	{
		// server close the socket, iocp will return read 0;
		FORMAT_DEBUG("TcpConnectionIocp::handleReadEvent() find that wsa read return 0, so we close socket");
		handleCloseEvent();
	}
	else
	{		
		// call read again
		preRead();
	}
}

void TcpConnectionIocp::handleCloseEvent()
{
	TcpConnection::handleCloseEvent();
	// cancel read, notice user that buffer should free
	SocketIocpPtr sock = std::dynamic_pointer_cast<SocketIocp>(localSocket);
	sock->clear();

}


