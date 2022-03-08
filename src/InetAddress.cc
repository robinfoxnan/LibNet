#include "../include/CommonHeader.h"
#include "../include/CommonLog.h"
#include "../include/InetAddress.h"
#include "../include/SocketsApi.h"

#if defined(_WIN32) || defined(_WIN64)
#include "../include/windows/Endian.h"
#else
#include "../include/linux/Endian.h"
#endif


// INADDR_ANY use (type)value casting.
//#pragma GCC diagnostic ignored "-Wold-style-cast"
static const in_addr_t kInaddrAny = INADDR_ANY;
static const in_addr_t kInaddrLoopback = INADDR_LOOPBACK;
//#pragma GCC diagnostic error "-Wold-style-cast"

//     /* Structure describing an Internet socket address.  */
//     struct sockaddr_in {
//         sa_family_t    sin_family; /* address family: AF_INET */
//         uint16_t       sin_port;   /* port in network byte order */
//         struct in_addr sin_addr;   /* internet address */
//     };

//     /* Internet address. */
//     typedef uint32_t in_addr_t;
//     struct in_addr {
//         in_addr_t       s_addr;     /* address in network byte order */
//     };

//     struct sockaddr_in6 {
//         sa_family_t     sin6_family;   /* address family: AF_INET6 */
//         uint16_t        sin6_port;     /* port in network byte order */
//         uint32_t        sin6_flowinfo; /* IPv6 flow information */
//         struct in6_addr sin6_addr;     /* IPv6 address */
//         uint32_t        sin6_scope_id; /* IPv6 scope-id */
//     };

using namespace LibNet;

static_assert(sizeof(InetAddress) == sizeof(struct sockaddr_in6),
              "InetAddress is same size as sockaddr_in6");
static_assert(offsetof(sockaddr_in, sin_family) == 0, "sin_family offset 0");
static_assert(offsetof(sockaddr_in6, sin6_family) == 0, "sin6_family offset 0");
static_assert(offsetof(sockaddr_in, sin_port) == 2, "sin_port offset 2");
static_assert(offsetof(sockaddr_in6, sin6_port) == 2, "sin6_port offset 2");

InetAddress::InetAddress(uint16_t portArg, bool loopbackOnly, bool ipv6)
{
  static_assert(offsetof(InetAddress, addr6_) == 0, "addr6_ offset 0");
  static_assert(offsetof(InetAddress, addr_) == 0, "addr_ offset 0");
  if (ipv6)
  {
    memset(&addr6_, 0, sizeof addr6_);
    addr6_.sin6_family = AF_INET6;
    in6_addr ip = loopbackOnly ? in6addr_loopback : in6addr_any;
    addr6_.sin6_addr = ip;
    addr6_.sin6_port = socketsApi::hostToNetwork16(portArg);
  }
  else
  {
    memset(&addr_, 0,  sizeof addr_);
    addr_.sin_family = AF_INET;
    in_addr_t ip = loopbackOnly ? kInaddrLoopback : kInaddrAny;
    addr_.sin_addr.s_addr = socketsApi::hostToNetwork32(ip);
    addr_.sin_port = socketsApi::hostToNetwork16(portArg);
  }
}

InetAddress::InetAddress(const string& ip, uint16_t portArg, bool ipv6)
{
  if (ipv6 || strchr(ip.c_str(), ':'))
  {
    memset(&addr6_, 0, sizeof addr6_);
    socketsApi::fromIpPort(ip.c_str(), portArg, &addr6_);
  }
  else
  {
    memset(&addr_, 0, sizeof addr_);
    socketsApi::fromIpPort(ip.c_str(), portArg, &addr_);
  }
}

string InetAddress::toIpPort() const
{
  char buf[64] = "";
  socketsApi::toIpPort(buf, sizeof buf, getSockAddr());
  return buf;
}

string InetAddress::toIp() const
{
  char buf[64] = "";
  socketsApi::toIp(buf, sizeof buf, getSockAddr());
  return buf;
}

uint32_t InetAddress::ipv4NetEndian() const
{
  assert(family() == AF_INET);
  return addr_.sin_addr.s_addr;
}

uint16_t InetAddress::port() const
{
  return socketsApi::networkToHost16(portNetEndian());
}

// used for formatting string
static thread_local char t_resolveBuffer[64 * 1024];

bool InetAddress::resolve(const string & hostname, InetAddress* out)
{

#if defined(_WIN32) || defined(_WIN64)
	struct addrinfo hints;
	struct addrinfo *res, *cur;
	struct sockaddr_in *addr;

	memset(&hints, 0, sizeof(addrinfo));
	hints.ai_family = AF_INET;	//IPv4
	hints.ai_flags = AI_PASSIVE; //匹配所有 IP 地址
	hints.ai_protocol = 0;        //匹配所有协议
	hints.ai_socktype = SOCK_STREAM; //流类型

	//获取 ip address,  res 指向一个链表Address Information链表
	int ret = getaddrinfo(hostname.c_str(), NULL, &hints, &res);
	if (ret == -1)
	{
		perror("getaddrinfo");
		return false;
	}
	for (cur = res; cur != NULL; cur = cur->ai_next)
	{
		addr = (struct sockaddr_in *) cur->ai_addr; //获取当前 address

		out->addr_.sin_addr = addr->sin_addr;

		//char m_IpAddr[16];
		//sprintf(m_IpAddr, "%d.%d.%d.%d", addr->sin_addr.S_un.S_un_b.s_b1,
		//	addr->sin_addr.S_un.S_un_b.s_b2,
		//	addr->sin_addr.S_un.S_un_b.s_b3,
		//	addr->sin_addr.S_un.S_un_b.s_b4);

		//printf("%s\n", m_IpAddr); //输出到控制台
}
	return true;
  
#else
	assert(out != NULL);
	struct hostent hent;
	struct hostent* he = NULL;
	int herrno = 0;
	memset(&hent, 0, sizeof(hent));
	int ret = 0;
  he = gethostbyname(hostname.c_str());
  ret = gethostbyname_r(hostname.c_str(), &hent, t_resolveBuffer, sizeof t_resolveBuffer, &he, &herrno);

  if (ret == 0 && he != NULL)
  {
    assert(he->h_addrtype == AF_INET && he->h_length == sizeof(uint32_t));
    out->addr_.sin_addr = *reinterpret_cast<struct in_addr*>(he->h_addr);
    return true;
  }
  else
  {
    if (ret)
    {
      FORMAT_ERROR("InetAddress::resolve");
    }
    return false;
  }
#endif
}

void InetAddress::setScopeId(uint32_t scope_id)
{
  if (family() == AF_INET6)
  {
    addr6_.sin6_scope_id = scope_id;
  }
}
