#include "../include/CommonHeader.h"
#include "../include/Socket.h"
#include "../include/SocketsApi.h"
#include "../include/InetAddress.h"
#include "../include/SocketsApi.h"
#include "../include/CommonLog.h"

using namespace LibNet;

Socket::Socket(IOEventLoop *loop, int fd) : Channel(loop, fd)
{

}
Socket::~Socket()
{
	if (fd_ != INVALID_SOCKET)
		socketsApi::close(fd_);
}

// int Socket::open(sa_family_t family)
// {
//     this->fd_ = socketsApi::createNonblockingOrDie(family);
// }

ssize_t Socket::read(char *buf, size_t count, void *param)
{
	return socketsApi::read(fd_, (char*)buf, count);
}

ssize_t Socket::write( const char *buf, size_t count, void *param)
{
	return socketsApi::write(fd_, (char*)buf, count);
}

bool Socket::isValid()
{
	return (fd_ != INVALID_SOCKET);
}

void Socket::setBlockMode(bool bBlock)
{
	socketsApi::setNonBlockAndCloseOnExec(fd_, bBlock);
}

int Socket::connect(const InetAddress& peerAddr, bool bBlock)
{
	if (fd_ == INVALID_SOCKET)
	{
		this->fd_ = socketsApi::createNonblockingOrDie(peerAddr.family());
	}

	if (bBlock)
	{
		socketsApi::setNonBlockAndCloseOnExec(fd_, true);
	}
    return socketsApi::connect(this->fd_, peerAddr.getSockAddr());
}

int Socket::close()
{
    socketsApi::close(fd_);
    fd_ = INVALID_SOCKET;
    return 0;

}
void Socket::setSendTimeout(int secs)
{
	socketsApi::setSendTimeout(fd_, secs);
}
void Socket::setRecvTimeout(int secs)
{
	socketsApi::setRecvTimeout(fd_, secs);
}

bool Socket::getTcpInfo(struct tcp_info* tcpi) const
{
#if defined (WIN32) || defined(_WIN64)
	return false;
#else
  socklen_t len = sizeof(*tcpi);
  memset(tcpi, 0, len);
  return ::getsockopt(fd_, SOL_TCP, TCP_INFO, tcpi, &len) == 0;
#endif
}

bool Socket::getTcpInfoString(char* buf, int len) const
{
#if defined (WIN32) || defined(_WIN64)
	return false;
#else
  struct tcp_info tcpi;
  bool ok = getTcpInfo(&tcpi);
  if (ok)
  {
    snprintf(buf, len, "unrecovered=%u "
             "rto=%u ato=%u snd_mss=%u rcv_mss=%u "
             "lost=%u retrans=%u rtt=%u rttvar=%u "
             "sshthresh=%u cwnd=%u total_retrans=%u",
             tcpi.tcpi_retransmits,  // Number of unrecovered [RTO] timeouts
             tcpi.tcpi_rto,          // Retransmit timeout in usec
             tcpi.tcpi_ato,          // Predicted tick of soft clock in usec
             tcpi.tcpi_snd_mss,
             tcpi.tcpi_rcv_mss,
             tcpi.tcpi_lost,         // Lost packets
             tcpi.tcpi_retrans,      // Retransmitted packets out
             tcpi.tcpi_rtt,          // Smoothed round trip time in usec
             tcpi.tcpi_rttvar,       // Medium deviation
             tcpi.tcpi_snd_ssthresh,
             tcpi.tcpi_snd_cwnd,
             tcpi.tcpi_total_retrans);  // Total retransmits for entire connection
  }
  return ok;
#endif
}

void Socket::bindAddress(const InetAddress& addr)
{
  socketsApi::bindOrDie(fd_, addr.getSockAddr());
}

void Socket::listen()
{
  socketsApi::listenOrDie(fd_);
}

int Socket::accept(InetAddress* peeraddr)
{
  struct sockaddr_in6 addr;
  memset(&addr, 0, sizeof addr);
  int connfd = socketsApi::accept(fd_, &addr);
  if (connfd >= 0)
  {
    peeraddr->setSockAddrInet6(addr);
  }
  return connfd;
}

void Socket::shutdownWrite()
{
  socketsApi::shutdownWrite(fd_);
}

void Socket::setTcpNoDelay(bool on)
{
  int optval = on ? 1 : 0;
  ::setsockopt(fd_, 
	  IPPROTO_TCP, 
	  TCP_NODELAY,
	  reinterpret_cast<char *>(&optval), 
	  static_cast<socklen_t>(sizeof optval));
  // FIXME CHECK
}

void Socket::setReuseAddr(bool on)
{
  int optval = on ? 1 : 0;
  ::setsockopt(fd_, 
	  SOL_SOCKET, 
	  SO_REUSEADDR,
	  reinterpret_cast<char *>(&optval),
	  static_cast<socklen_t>(sizeof optval));
  // FIXME CHECK
}

void Socket::setReusePort(bool on)
{
#ifdef SO_REUSEPORT
  int optval = on ? 1 : 0;
  int ret = ::setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT,
                         &optval, static_cast<socklen_t>(sizeof optval));
  if (ret < 0 && on)
  {
    FORMAT_ERROR("SO_REUSEPORT failed.");
  }
#else

#if defined (WIN32) || defined(_WIN64)
	if (on)
	{
		FORMAT_INFO("SO_REUSEADDR is same with linux on windows platform.");
	}
#else
	if (on)
	{
		FORMAT_ERROR("SO_REUSEPORT is not supported.");
	}
#endif
  
#endif
}

void Socket::setKeepAlive(bool on)
{
  int optval = on ? 1 : 0;
  ::setsockopt(fd_, 
	  SOL_SOCKET, 
	  SO_KEEPALIVE, 
	  reinterpret_cast<char *>(&optval), 
	  static_cast<socklen_t>(sizeof optval));
  // FIXME CHECK
}

int Socket::getLastErr()
{
	errCode  = socketsApi::getSocketError(fd_);
	return errCode;
}

SOCK_STATUS Socket::waitOrClose()
{
	return socketsApi::waitOrClose(errCode);
}

string Socket::getErrorInfo()
{
	return socketsApi::getErrorInfo(errCode);
}

