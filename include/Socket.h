#ifndef LibNet_NET_SOCKET_H
#define LibNet_NET_SOCKET_H

#include "./Channel.h"
#include "./InetAddress.h"
#include "./SocketsApi.h"

// struct tcp_info is in <netinet/tcp.h>
struct tcp_info;

namespace LibNet
{
	class InetAddress;

	
	class Socket : public Channel
	{
	public:
		Socket(IOEventLoop *loop, int fd);
		virtual ~Socket() override;

		//int open(sa_family_t family = AF_INET);
		virtual int connect(const InetAddress& peerAddr, bool bBlock = false);
		virtual int close();
		virtual ssize_t read(char *buf, size_t count, void *param = nullptr);
		virtual ssize_t write(const char *buf, size_t count, void *param = nullptr);

		// these three is for windows iocp
		virtual ssize_t readEnd(char *buf, size_t count) { return 0; };  // return data here
		virtual ssize_t readEnd(char **buf, void **param = nullptr)  { return 0; }                 // tcpconection use it 
		virtual ssize_t writeEnd(char **buf, void **param = nullptr) { return 0; }

		virtual int getLastErr();
		virtual string getErrorInfo();
		virtual SOCK_STATUS waitOrClose();

		bool isValid();

		// return true if success.
		bool getTcpInfo(struct tcp_info *) const;
		bool getTcpInfoString(char *buf, int len) const;

		
		void bindAddress(const InetAddress &localaddr);
		void listen();


		int accept(InetAddress *peeraddr);


		void shutdownWrite();
		void setSendTimeout(int secs);
		void setRecvTimeout(int secs);
		void setTcpNoDelay(bool on);
		void setReuseAddr(bool on);
		void setReusePort(bool on);
		void setKeepAlive(bool on);
		void setBlockMode(bool bBlock);

	protected:
		int errCode;

	private:
		Socket(const Socket &) = delete;
		Socket(Socket &&) = delete;
	};

} // namespace LibNet

#endif // LibNet_NET_SOCKET_H
