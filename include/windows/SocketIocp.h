#pragma once
#include "../Channel.h"
#include "../InetAddress.h"
#include "../Socket.h"
#include "../SocketsApi.h"
#include "./IocpContex.h"
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

namespace LibNet
{
	class SocketIocp : public Socket
	{
	public:
		SocketIocp(IOEventLoop *loop, int fd);
		virtual ~SocketIocp();

		virtual int connect(const InetAddress& peerAddr, bool bBlock = false) override;
		virtual int close() override;
		virtual ssize_t read(char *buf, size_t count, void *param = nullptr) override;
		virtual ssize_t write(const char *buf, size_t count, void *param = nullptr) override;

		virtual ssize_t readEnd(char *buf, size_t count) override;  // return data here
		virtual ssize_t readEnd(char **buf, void **param = nullptr) override;               // tcpconection use it 
		virtual ssize_t writeEnd(char **buf, void **param = nullptr) override;

		void clear();

	private:
		std::deque<IocpContext> readQue;
		std::deque<IocpContext> writeQue;

		std::mutex mutexRead;
		std::mutex mutexWrite;
	};

	using SocketIocpPtr = std::shared_ptr<SocketIocp>;
}