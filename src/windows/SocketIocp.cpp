#include "../../include/CommonLog.h"
#include "../../include/windows/SocketIocp.h"
#include "../../include/windows/IocpContex.h"

using namespace LibNet;

SocketIocp::SocketIocp(IOEventLoop *loop, int fd): Socket(loop, fd)
{
	/*bool ret = init();
	(void)ret;
	FORMAT_DEBUG("init ssl ret = %d", (int)ret);*/
}
SocketIocp::~SocketIocp()
{

}
// only use syn mode
int SocketIocp::connect(const InetAddress& peerAddr, bool bBlock)
{
	return Socket::connect(peerAddr, true);
}

int SocketIocp::close()
{
	return Socket::close();
}

// start a asyn mode read && 
ssize_t SocketIocp::read(char *buf, size_t count, void *param)
{
	std::lock_guard<mutex> guard(mutexRead);
	readQue.emplace_back((Channel *)this, (char *)buf, count, IOCP_TYPE::IORead, param);
	size_t index = readQue.size() - 1;

	DWORD    dwIoSize = 0;
	ULONG	 ulFlags = 0; //MSG_PARTIAL;
	int     nRetVal = WSARecv(fd_,
		& readQue[index].wsabuf,
		1,
		&dwIoSize,
		&ulFlags,
		&readQue[index].overlapped,
		NULL);

	if (nRetVal == SOCKET_ERROR)
	{
		this->errCode = WSAGetLastError();
		if (this->errCode == WSA_IO_PENDING)
			return 0;

		FORMAT_DEBUG("WSARecv meets error= %d, %s", errCode, getErrorInfo().c_str());
		return -1;
	}
	assert(nRetVal == 0);

	return dwIoSize;
}
// readEnd for getData


// just for openssl bio
ssize_t SocketIocp::readEnd(char *buf, size_t count)
{
	return 0;
}

// for upper level
ssize_t SocketIocp::readEnd(char **buf, void **param)
{
	std::lock_guard<mutex> guard(mutexRead);

	// usually, size == 1, should be i==0
	if (readQue.size() > 0)
	{
		if (readQue[0].operation == IOCP_TYPE::IODONE)
		{
			ssize_t len = readQue[0].ioSize;
			*buf = readQue[0].wsabuf.buf;
			if (param)
				*param = readQue[0].param;

			readQue.pop_front();
			return len;
		}
	}

	// must never meet here
	*buf = nullptr;
	return -1;
}

// see https://www.cnblogs.com/yifen/archive/2013/01/19/2867860.html
// If no error occurs and the send operation has completed immediately, 
// WSASend returns zero. 
// Otherwise, a value of SOCKET_ERROR is returned, and a specific error code 
//can be retrieved by callingWSAGetLastError. 
//The error code WSA_IO_PENDING indicates that the overlapped operation has been 
//successfully initiated and that completion will be indicated at a later time. 
//Any other error code indicates that the overlapped operation was not successfully 
//initiated and no completion indication will occur.
ssize_t SocketIocp::write(const char *buf, size_t count, void *param)
{
	if (fd_ == INVALID_SOCKET)
		return -1;

	// must push in que first; when  IOCP is faster, he will can't find data in que!!
	std::lock_guard<mutex> guard(mutexWrite);
	writeQue.emplace_back((Channel *)this, (char *)buf, count, IOCP_TYPE::IOSend, param);
	ssize_t index = writeQue.size() - 1;
	IocpContext & context = writeQue[index];

	DWORD    dwIoSize = 0;
	ULONG	 ulFlags = 0; //MSG_PARTIAL;

	int      nRetVal  = WSASend(fd_,
		&context.wsabuf,
		1,
		&dwIoSize,
		ulFlags,
		&context.overlapped,
		NULL);

	
	if (nRetVal == SOCKET_ERROR)
	{
		this->errCode = WSAGetLastError();
		if (this->errCode == WSA_IO_PENDING)
		{
			return 0;
		}
		// pop it 
		writeQue.pop_back();

		FORMAT_DEBUG("WSASend meets error= %d, %s", errCode, getErrorInfo().c_str());
		return -1;
	}

	assert(nRetVal == 0);

	return dwIoSize;
}

// let user to know, it's time to free buf
ssize_t SocketIocp::writeEnd(char **buf, void **param)
{
	std::lock_guard<mutex> guard(mutexWrite);
	// usually, should be i==0
	if (writeQue.size() > 0)
	{
		if (writeQue[0].operation == IOCP_TYPE::IODONE)
		{
			ssize_t ioLen = writeQue[0].ioSize;
			ssize_t bufLen = writeQue[0].wsabuf.len;

			*buf = writeQue[0].wsabuf.buf;
			if (param)
				*param = writeQue[0].param;

			writeQue.pop_front();
			return (ioLen == bufLen) ? ioLen : 0;
		}
	}

	// something wrong, must check code!!
	*buf = nullptr;
	return -1;

}

void SocketIocp::clear()
{
	printf("SocketIocp write queue len = %zu", writeQue.size());
	std::lock_guard<mutex> guard1(mutexWrite);
	writeQue.clear();

	printf("SocketIocp read queue len = %zu", readQue.size());
	std::lock_guard<mutex> guard2(mutexRead);
	readQue.clear();
}



