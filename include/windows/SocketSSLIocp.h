#pragma once
#include "./SocketIocp.h"

#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

namespace LibNet
{
	class SocketSSLIocp : public SocketIocp
	{
	public:
		SocketSSLIocp(IOEventLoop *loop, int fd);
		virtual ~SocketSSLIocp();

		virtual int connect(const InetAddress& peerAddr, bool bBlock = false) override;
		virtual int close() override;
		virtual ssize_t read(char *buf, size_t count, void *param = nullptr) override;
		virtual ssize_t write(const char *buf, size_t count, void *param = nullptr) override;

		virtual int getLastErr()  override;
		virtual string getErrorInfo() override;
		virtual SOCK_STATUS waitOrClose() override;

		// used by BIO
		int recvRaw(char * buf, size_t len);
		int sendRaw(const char * buf, size_t len);

	private:
		bool init();

		SSL_CTX *ctx;
		SSL* ssl;
		int errSSL;
	};
}