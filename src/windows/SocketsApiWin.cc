#include "../../include/CommonHeader.h"
#include "../../include/CommonLog.h"
#include "../../include/SocketsApi.h"
#include "../../include/windows/SocketInit.h"

using namespace LibNet;
//using namespace LibNet::socketsApi;

namespace
{
	typedef struct sockaddr SA;

	static WSInit   winSocketInit;
}
//////////////////////////////////////////////////////////////////////////////
const struct sockaddr *socketsApi::sockaddr_cast(const struct sockaddr_in6 *addr)
{
	return static_cast<const struct sockaddr *>(static_cast<const void *>(addr));
}

struct sockaddr *socketsApi::sockaddr_cast(struct sockaddr_in6 *addr)
{
	return static_cast<struct sockaddr *>(static_cast<void *>(addr));
}

const struct sockaddr *socketsApi::sockaddr_cast(const struct sockaddr_in *addr)
{
	return static_cast<const struct sockaddr *>(static_cast<const void *>(addr));
}

const struct sockaddr_in *socketsApi::sockaddr_in_cast(const struct sockaddr *addr)
{
	return static_cast<const struct sockaddr_in *>(static_cast<const void *>(addr));
}

const struct sockaddr_in6 *socketsApi::sockaddr_in6_cast(const struct sockaddr *addr)
{
	return static_cast<const struct sockaddr_in6 *>(static_cast<const void *>(addr));
}

//////////////////////////////////////////////////////////////////////////////////////////
int socketsApi::createNonblockingOrDie(sa_family_t family)
{
	//socket_t sockfd = ::socket(family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
	socket_t sockfd = WSASocketW(family, SOCK_STREAM, IPPROTO_TCP, nullptr, 0,
		WSA_FLAG_NO_HANDLE_INHERIT | WSA_FLAG_OVERLAPPED);
	if (sockfd < 0)
	{
		 FORMAT_ERROR("socketsApiWin::createNonblockingOrDie");
	}

	u_long type = 1;
	ioctlsocket(sockfd, FIONBIO, &type);

	return sockfd;
}

void socketsApi::bindOrDie(socket_t sockfd, const struct sockaddr *addr)
{
	int ret = ::bind(sockfd, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
	if (ret < 0)
	{
		FORMAT_ERROR("socketsApiWin::bindOrDie");
	}
}

void socketsApi::listenOrDie(socket_t sockfd)
{
	int ret = ::listen(sockfd, SOMAXCONN);
	if (ret < 0)
	{
		FORMAT_ERROR("socketsApi::listenOrDie");
	}
}

void socketsApi::setSendTimeout(socket_t sockfd, int secs)
{
	struct timeval timeo = { secs,  0 };
	socklen_t len = sizeof(timeo);

	setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeo, len);
}
void socketsApi::setRecvTimeout(socket_t sockfd, int secs)
{
	struct timeval timeo = { secs,  0 };
	socklen_t len = sizeof(timeo);

	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeo, len);
}

void socketsApi::setNonBlockAndCloseOnExec(socket_t sockfd, bool bBlock)
{
	// non-block
	unsigned long ul = 1;
	if (bBlock)
		ul = 0;

	int ret = ioctlsocket(sockfd, FIONBIO, (unsigned long *)&ul);
	if (ret == SOCKET_ERROR)  
	{
		FORMAT_ERROR("windows setNonBlockAndCloseOnExec error");
	}

	(void)ret;
}

int socketsApi::accept(socket_t sockfd, struct sockaddr_in6 *addr)
{
	socklen_t addrlen = static_cast<socklen_t>(sizeof *addr);

	socket_t connfd = ::accept(sockfd, sockaddr_cast(addr), &addrlen);
	setNonBlockAndCloseOnExec(connfd);

	return connfd;
}

int socketsApi::connect(socket_t sockfd, const struct sockaddr *addr)
{
	return ::connect(sockfd, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
}

ssize_t socketsApi::read(socket_t sockfd, char *buf, size_t count)
{
	return ::recv(sockfd, (char*)buf, count, 0);
}

ssize_t socketsApi::write(socket_t sockfd, const char *buf, size_t count)
{
	return ::send(sockfd, (char*)buf, count, 0);
}

void socketsApi::close(socket_t sockfd)
{
	if (::closesocket(sockfd) < 0)
	{
		FORMAT_ERROR("socketsApi::close");
	}
}

void socketsApi::shutdownWrite(socket_t sockfd)
{
	// SD_RECEIVE，SD_SEND, SD_BOTH
	if (::shutdown(sockfd, SD_BOTH) < 0)
	{
		FORMAT_ERROR("socketsApi::shutdownWrite");
	}
}

void socketsApi::toIpPort(char *buf, size_t size,
	const struct sockaddr *addr)
{
	if (addr->sa_family == AF_INET6)
	{
		buf[0] = '[';
		toIp(buf + 1, size - 1, addr);
		size_t end = ::strlen(buf);
		const struct sockaddr_in6 *addr6 = sockaddr_in6_cast(addr);
		uint16_t port = socketsApi::networkToHost16(addr6->sin6_port);
		assert(size > end);
		snprintf(buf + end, size - end, "]:%u", port);
		return;
	}
	toIp(buf, size, addr);
	size_t end = ::strlen(buf);
	const struct sockaddr_in *addr4 = sockaddr_in_cast(addr);
	uint16_t port = socketsApi::networkToHost16(addr4->sin_port);
	assert(size > end);
	snprintf(buf + end, size - end, ":%u", port);
}

void socketsApi::toIp(char *buf, size_t size,
	const struct sockaddr *addr)
{
	if (addr->sa_family == AF_INET)
	{
		assert(size >= INET_ADDRSTRLEN);
		const struct sockaddr_in *addr4 = sockaddr_in_cast(addr);
		::inet_ntop(AF_INET, &addr4->sin_addr, buf, static_cast<socklen_t>(size));
	}
	else if (addr->sa_family == AF_INET6)
	{
		assert(size >= INET6_ADDRSTRLEN);
		const struct sockaddr_in6 *addr6 = sockaddr_in6_cast(addr);
		::inet_ntop(AF_INET6, &addr6->sin6_addr, buf, static_cast<socklen_t>(size));
	}
}

void socketsApi::fromIpPort(const char *ip, uint16_t port,
	struct sockaddr_in *addr)
{
	addr->sin_family = AF_INET;
	addr->sin_port = hostToNetwork16(port);
	if (::inet_pton(AF_INET, ip, &addr->sin_addr) <= 0)
	{
		FORMAT_ERROR("socketsApi::fromIpPort");
	}
}

void socketsApi::fromIpPort(const char *ip, uint16_t port,
	struct sockaddr_in6 *addr)
{
	addr->sin6_family = AF_INET6;
	addr->sin6_port = hostToNetwork16(port);
	if (::inet_pton(AF_INET6, ip, &addr->sin6_addr) <= 0)
	{
		FORMAT_ERROR("socketsApi::fromIpPort");
	}
}



struct sockaddr_in6 socketsApi::getLocalAddr(socket_t sockfd)
{
	struct sockaddr_in6 localaddr;
	memset(&localaddr, 0, sizeof localaddr);
	socklen_t addrlen = static_cast<socklen_t>(sizeof localaddr);
	if (::getsockname(sockfd, sockaddr_cast(&localaddr), &addrlen) < 0)
	{
		FORMAT_ERROR("socketsApi::getLocalAddr");
	}
	return localaddr;
}

struct sockaddr_in6 socketsApi::getPeerAddr(socket_t sockfd)
{
	struct sockaddr_in6 peeraddr;
	memset(&peeraddr, 0, sizeof peeraddr);
	socklen_t addrlen = static_cast<socklen_t>(sizeof peeraddr);
	if (::getpeername(sockfd, sockaddr_cast(&peeraddr), &addrlen) < 0)
	{
		FORMAT_ERROR("socketsApi::getPeerAddr");
	}
	return peeraddr;
}

bool socketsApi::isSelfConnect(socket_t sockfd)
{
	struct sockaddr_in6 localaddr = getLocalAddr(sockfd);
	struct sockaddr_in6 peeraddr = getPeerAddr(sockfd);
	if (localaddr.sin6_family == AF_INET)
	{
		const struct sockaddr_in *laddr4 = reinterpret_cast<struct sockaddr_in *>(&localaddr);
		const struct sockaddr_in *raddr4 = reinterpret_cast<struct sockaddr_in *>(&peeraddr);
		return laddr4->sin_port == raddr4->sin_port && laddr4->sin_addr.s_addr == raddr4->sin_addr.s_addr;
	}
	else if (localaddr.sin6_family == AF_INET6)
	{
		return localaddr.sin6_port == peeraddr.sin6_port && memcmp(&localaddr.sin6_addr, &peeraddr.sin6_addr, sizeof localaddr.sin6_addr) == 0;
	}
	else
	{
		return false;
	}
}
int socketsApi::getSocketError(socket_t sockfd)
{
	return WSAGetLastError();
}

string socketsApi::getErrorInfo(int code)
{

	//char n_lpBuffer[256];

	char *ptr;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, 
		NULL, 
		code, 
		MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT),  // MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
		(LPTSTR)&ptr, 
		0, 
		NULL);
	string str = "unknow";


	if (ptr != NULL)
	{
		//mbstowcs(ptr, n_lpBuffer, strlen(lpBuffer));
		str = ptr;
		LocalFree(ptr);
	}

	return str;
}

// error -1
// close socket 0
// wait 1
SOCK_STATUS socketsApi::waitOrClose(int code)
{
	switch (code)
	{
	case 0:
		return SOCK_STATUS::SOCK_OK;

	case WSAEALREADY:
	case WSAEISCONN:       // alreaydy connected
	case WSAEINPROGRESS:   // 10036L
	case WSAEWOULDBLOCK:
		return SOCK_STATUS::SOCK_WAIT;

	case WSAEBADF:
	case WSAEADDRINUSE:
		return SOCK_STATUS::SOCK_ERROR;

	case WSAECONNABORTED:  // 10053
	case WSAECONNRESET:    // 10054
	default:
		return SOCK_STATUS::SOCK_ERROR;
	}
}



