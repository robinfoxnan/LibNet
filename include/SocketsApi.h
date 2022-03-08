#ifndef LibNet_NET_SOCKETSOPS_H
#define LibNet_NET_SOCKETSOPS_H

#include "CommonHeader.h"

#if defined(_WIN32) || defined(_WIN64)
#include "windows/Endian.h"
#else
#include "linux/Endian.h"
#endif

namespace LibNet
{

	typedef enum {
		SOCK_ERROR = -1,
		SOCK_OK = 0,
		SOCK_WAIT = 1,
		SOCK_RETRY = 2
	} SOCK_STATUS;

    namespace socketsApi
    {
        ///
        /// Creates a non-blocking socket file descriptor,
        /// abort if any error.
        int createNonblockingOrDie(sa_family_t family);

        int connect(socket_t sockfd, const struct sockaddr *addr);
        void bindOrDie(socket_t sockfd, const struct sockaddr *addr);

		void setSendTimeout(socket_t sockfd, int secs);
		void setRecvTimeout(socket_t sockfd, int secs);

        void listenOrDie(socket_t sockfd);
        int accept(socket_t sockfd, struct sockaddr_in6 *addr);
        ssize_t read(socket_t sockfd, char *buf, size_t count);
       // ssize_t readv(socket_t sockfd, const struct iovec *iov, int iovcnt);
        ssize_t write(socket_t sockfd, const char *buf, size_t count);
        void close(socket_t sockfd);
        void shutdownWrite(socket_t sockfd);

        void toIpPort(char *buf, size_t size, const struct sockaddr *addr);
        void toIp(char *buf, size_t size,  const struct sockaddr *addr);

        void fromIpPort(const char *ip, uint16_t port, struct sockaddr_in *addr);
        void fromIpPort(const char *ip, uint16_t port, struct sockaddr_in6 *addr);

        int getSocketError(socket_t sockfd);

        const struct sockaddr *sockaddr_cast(const struct sockaddr_in *addr);
        const struct sockaddr *sockaddr_cast(const struct sockaddr_in6 *addr);
        struct sockaddr *sockaddr_cast(struct sockaddr_in6 *addr);
        const struct sockaddr_in *sockaddr_in_cast(const struct sockaddr *addr);
        const struct sockaddr_in6 *sockaddr_in6_cast(const struct sockaddr *addr);

        struct sockaddr_in6 getLocalAddr(socket_t sockfd);
        struct sockaddr_in6 getPeerAddr(socket_t sockfd);
        bool isSelfConnect(socket_t sockfd);

		string getErrorInfo(int code);
		SOCK_STATUS waitOrClose(int code);
		void setNonBlockAndCloseOnExec(socket_t sockfd, bool bBlock=false);

// in Endian.h
//        int64_t hostToNetwork64(int64_t x);
//        int32_t hostToNetwork32(int32_t x);
//        int16_t hostToNetwork32(int16_t x);


    } // namespace sockets

} // namespace LibNet

#endif // LibNet_NET_SOCKETSOPS_H
