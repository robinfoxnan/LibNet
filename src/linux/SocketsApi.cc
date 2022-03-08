#include "../../include/SocketsApi.h"
#include "../../include/CommonHeader.h"
#include "../../include/CommonLog.h"
#include <stdio.h>  // snprintf

#if defined(_WIN32) || defined(_WIN64)

#else
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/uio.h>  // readv
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#endif

using namespace LibNet;
using namespace LibNet::socketsApi;

namespace
{

typedef struct sockaddr SA;


#if VALGRIND || defined (NO_ACCEPT4)
void setNonBlockAndCloseOnExec(int sockfd)
{
  // non-block
  int flags = ::fcntl(sockfd, F_GETFL, 0);
  flags |= O_NONBLOCK;
  int ret = ::fcntl(sockfd, F_SETFL, flags);
  // FIXME check

  // close-on-exec
  flags = ::fcntl(sockfd, F_GETFD, 0);
  flags |= FD_CLOEXEC;
  ret = ::fcntl(sockfd, F_SETFD, flags);
  // FIXME check

  (void)ret;
}
#endif

#pragma GCC diagnostic ignored "-Wold-style-cast"
  class IgnoreSigPipe
  {
  public:
    IgnoreSigPipe()
    {
      ::signal(SIGPIPE, SIG_IGN);
      // LOG_TRACE << "Ignore SIGPIPE";
    }
  };
#pragma GCC diagnostic error "-Wold-style-cast"

  IgnoreSigPipe initObj;

}  // namespace

const struct sockaddr* socketsApi::sockaddr_cast(const struct sockaddr_in6* addr)
{
  return static_cast<const struct sockaddr*>(static_cast<const void*>(addr));
}

struct sockaddr* socketsApi::sockaddr_cast(struct sockaddr_in6* addr)
{
  return static_cast<struct sockaddr*>(static_cast<void*>(addr));
}

const struct sockaddr* socketsApi::sockaddr_cast(const struct sockaddr_in* addr)
{
  return static_cast<const struct sockaddr*>(static_cast<const void*>(addr));
}

const struct sockaddr_in* socketsApi::sockaddr_in_cast(const struct sockaddr* addr)
{
  return static_cast<const struct sockaddr_in*>(static_cast<const void*>(addr));
}

const struct sockaddr_in6* socketsApi::sockaddr_in6_cast(const struct sockaddr* addr)
{
  return static_cast<const struct sockaddr_in6*>(static_cast<const void*>(addr));
}

int socketsApi::createNonblockingOrDie(sa_family_t family)
{
#if VALGRIND
  int sockfd = ::socket(family, SOCK_STREAM, IPPROTO_TCP);
  if (sockfd < 0)
  {
    LOG_SYSFATAL << "socketsApi::createNonblockingOrDie";
  }

  setNonBlockAndCloseOnExec(sockfd);
#else
  int sockfd = ::socket(family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
  if (sockfd < 0)
  {
      FORMAT_ERROR("socketsApi::createNonblockingOrDie error");
  }
#endif
  return sockfd;
}

void socketsApi::bindOrDie(int sockfd, const struct sockaddr* addr)
{
  int ret = ::bind(sockfd, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
  if (ret < 0)
  {
    FORMAT_ERROR("socketsApi::bindOrDie error");
  }
}

void socketsApi::listenOrDie(int sockfd)
{
  int ret = ::listen(sockfd, SOMAXCONN);
  if (ret < 0)
  {
    FORMAT_ERROR("socketsApi::listenOrDie error");
  }
}

int socketsApi::accept(int sockfd, struct sockaddr_in6* addr)
{
  socklen_t addrlen = static_cast<socklen_t>(sizeof *addr);
#if VALGRIND || defined (NO_ACCEPT4)
  int connfd = ::accept(sockfd, sockaddr_cast(addr), &addrlen);
  setNonBlockAndCloseOnExec(connfd);
#else
  int connfd = ::accept4(sockfd, sockaddr_cast(addr),
                         &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
#endif
  if (connfd < 0)
  {
    int savedErrno = errno;
    FORMAT_ERROR("Socket::accept");
    switch (savedErrno)
    {
      case EAGAIN:
      case ECONNABORTED:
      case EINTR:
      case EPROTO: // ???
      case EPERM:
      case EMFILE: // per-process lmit of open file desctiptor ???
        // expected errors
        errno = savedErrno;
        break;
      case EBADF:
      case EFAULT:
      case EINVAL:
      case ENFILE:
      case ENOBUFS:
      case ENOMEM:
      case ENOTSOCK:
      case EOPNOTSUPP:
        // unexpected errors
        FORMAT_ERROR("unexpected error of ::accept %d", savedErrno);
        break;
      default:
        FORMAT_ERROR("unknown error of ::accept %d", savedErrno);
        break;
    }
  }
  return connfd;
}

int socketsApi::connect(int sockfd, const struct sockaddr* addr)
{
  return ::connect(sockfd, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
}

ssize_t socketsApi::read(int sockfd, char *buf, size_t count)
{
  return ::read(sockfd, buf, count);
}

//ssize_t socketsApi::readv(int sockfd, const struct iovec *iov, int iovcnt)
//{
//  return ::readv(sockfd, iov, iovcnt);
//}

ssize_t socketsApi::write(int sockfd, const char *buf, size_t count)
{
  return ::write(sockfd, buf, count);
}

void socketsApi::close(int sockfd)
{
  if (::close(sockfd) < 0)
  {
    FORMAT_ERROR("socketsApi::close error");
  }
}

void socketsApi::shutdownWrite(int sockfd)
{
  if (::shutdown(sockfd, SHUT_WR) < 0)
  {
    FORMAT_ERROR("socketsApi::shutdownWrite");
  }
}

void socketsApi::toIpPort(char* buf, size_t size,
                       const struct sockaddr* addr)
{
  if (addr->sa_family == AF_INET6)
  {
    buf[0] = '[';
    toIp(buf+1, size-1, addr);
    size_t end = ::strlen(buf);
    const struct sockaddr_in6* addr6 = sockaddr_in6_cast(addr);
    uint16_t port = socketsApi::networkToHost16(addr6->sin6_port);
    assert(size > end);
    snprintf(buf+end, size-end, "]:%u", port);
    return;
  }
  toIp(buf, size, addr);
  size_t end = ::strlen(buf);
  const struct sockaddr_in* addr4 = sockaddr_in_cast(addr);
  uint16_t port = socketsApi::networkToHost16(addr4->sin_port);
  assert(size > end);
  snprintf(buf+end, size-end, ":%u", port);
}

void socketsApi::toIp(char* buf, size_t size,
                   const struct sockaddr* addr)
{
  if (addr->sa_family == AF_INET)
  {
    assert(size >= INET_ADDRSTRLEN);
    const struct sockaddr_in* addr4 = sockaddr_in_cast(addr);
    ::inet_ntop(AF_INET, &addr4->sin_addr, buf, static_cast<socklen_t>(size));
  }
  else if (addr->sa_family == AF_INET6)
  {
    assert(size >= INET6_ADDRSTRLEN);
    const struct sockaddr_in6* addr6 = sockaddr_in6_cast(addr);
    ::inet_ntop(AF_INET6, &addr6->sin6_addr, buf, static_cast<socklen_t>(size));
  }
}

void socketsApi::fromIpPort(const char* ip, uint16_t port,
                         struct sockaddr_in* addr)
{
  addr->sin_family = AF_INET;
  addr->sin_port = hostToNetwork16(port);
  if (::inet_pton(AF_INET, ip, &addr->sin_addr) <= 0)
  {
    FORMAT_ERROR("socketsApi::fromIpPort");
  }
}

void socketsApi::fromIpPort(const char* ip, uint16_t port,
                         struct sockaddr_in6* addr)
{
  addr->sin6_family = AF_INET6;
  addr->sin6_port = hostToNetwork16(port);
  if (::inet_pton(AF_INET6, ip, &addr->sin6_addr) <= 0)
  {
    FORMAT_ERROR("socketsApi::fromIpPort");
  }
}

int socketsApi::getSocketError(int sockfd)
{
  int optval;
  socklen_t optlen = static_cast<socklen_t>(sizeof optval);

  if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
  {
    return errno;
  }
  else
  {
    return optval;
  }
}

struct sockaddr_in6 socketsApi::getLocalAddr(int sockfd)
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

struct sockaddr_in6 socketsApi::getPeerAddr(int sockfd)
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

bool socketsApi::isSelfConnect(int sockfd)
{
  struct sockaddr_in6 localaddr = getLocalAddr(sockfd);
  struct sockaddr_in6 peeraddr = getPeerAddr(sockfd);
  if (localaddr.sin6_family == AF_INET)
  {
    const struct sockaddr_in* laddr4 = reinterpret_cast<struct sockaddr_in*>(&localaddr);
    const struct sockaddr_in* raddr4 = reinterpret_cast<struct sockaddr_in*>(&peeraddr);
    return laddr4->sin_port == raddr4->sin_port
        && laddr4->sin_addr.s_addr == raddr4->sin_addr.s_addr;
  }
  else if (localaddr.sin6_family == AF_INET6)
  {
    return localaddr.sin6_port == peeraddr.sin6_port
        && memcmp(&localaddr.sin6_addr, &peeraddr.sin6_addr, sizeof localaddr.sin6_addr) == 0;
  }
  else
  {
    return false;
  }
}

void socketsApi::setNonBlockAndCloseOnExec(socket_t sockfd, bool bBlock)
{
    if (bBlock)
    {
        int old_option = ::fcntl(sockfd, F_GETFL );
        int new_option = old_option & (~O_NONBLOCK);
        fcntl(sockfd, F_SETFL, new_option );
    }
    else  // non-block
    {
        int old_option = ::fcntl(sockfd, F_GETFL );
        int new_option = old_option | O_NONBLOCK;
        fcntl(sockfd, F_SETFL, new_option );
    }

}

void socketsApi::setSendTimeout(socket_t sockfd, int secs)
{

}

void socketsApi::setRecvTimeout(socket_t sockfd, int secs)
{

}

SOCK_STATUS socketsApi::waitOrClose(int code)
{
    switch (code)
    {
    case 0:
    case EISCONN:
        return SOCK_STATUS::SOCK_OK;

    case EALREADY:
    case EINPROGRESS:   // connecting
           return SOCK_STATUS::SOCK_WAIT;

    case ECONNREFUSED:   // connect
    case EAGAIN:
      case ECONNABORTED:
      case EINTR:
      case EPROTO: // ???
      case EPERM:
      case EMFILE: // per-process lmit of open file desctiptor ???
        // expected errors


      case EBADF:
      case EFAULT:
      case EINVAL:
      case ENFILE:
      case ENOBUFS:
      case ENOMEM:
      case ENOTSOCK:
      case EOPNOTSUPP:
        return SOCK_STATUS::SOCK_ERROR;

    default:
        return SOCK_STATUS::SOCK_ERROR;

    }
}

string socketsApi::getErrorInfo(int code)
{
    return "";
}
