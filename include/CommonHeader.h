#pragma once
// std::*
#include <string>
#include <vector>
#include <thread>
#include <map>
#include <memory>
#include <deque>
#include <iostream>
#include <functional>
#include <atomic>
#include <mutex>
#include <condition_variable>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <assert.h>
#include <algorithm>
#include <vector>

#include <assert.h>
#include <cassert>
#include <sys/types.h>
#include <inttypes.h>
//#include "CharVector.h"
#include <errno.h>
#include <sstream>

#define Timestamp int64_t

using namespace std;

// choose log lib here
//#define LOG4CPP true
#define LOG4Z true


//namespace robin
//{
//	// used in Async Event callback queue
//	using DefaultCallback = std::function<void()>;


//}
#define DEFAUTL_BUF_SZ 1024 *64
#define DEFAULT_VEC_SZ 8192
//#define LOG4CPP true


#if defined(_MSC_VER)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

////////////////////////////////////////////////
#if defined (WIN32) || defined(_WIN64)
//#include <Winsock.h>
#include <winsock2.h>
#include <ws2tcpip.h>

using in_addr_t = ULONG;


using socket_t = SOCKET;
using sa_family = ADDRESS_FAMILY;
using sa_family_t = ADDRESS_FAMILY;
#define POLLRDHUP POLLHUP  

// static lib
#pragma comment(lib,"ws2_32.lib")
#pragma comment (lib,"Advapi32.lib")
#pragma comment (lib,"Iphlpapi.lib")
#pragma comment(lib, "Psapi.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "userenv.lib")

#else  // linux
using socket_t = int;

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#define _atoi64(val)     strtoll(val, NULL, 10)

#include <sys/eventfd.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#define INVALID_SOCKET -1
#endif

// enable or disable debug details
//#define DEBUG_PRINT(...)  printf(__VA_ARGS__)
#ifndef DEBUG_PRINT
#define DEBUG_PRINT(...)
#endif // DEBUG_PRINT

#define  WITH_OPENSSL true
//#define  MAC_OS       true

enum POLLERTYPE
{
    SELECT = 0,
    POLL,
    EPOLL,
    IOCP,
    KQUEUE,
    EPORT
};

//const POLLERTYPE poller_type = SELECT;
//const POLLERTYPE poller_type = POLL;
const POLLERTYPE poller_type = EPOLL;
//const POLLERTYPE poller_type = IOCP;


#ifdef WITH_OPENSSL
#if defined(_WIN32) || (_WIN64)
#pragma comment(lib, "openssl.lib")
#pragma comment(lib, "libcrypto.lib")
#pragma comment(lib, "libssl.lib")
#endif
#endif



