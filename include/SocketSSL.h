#pragma once
#include "./Channel.h"
#include "./InetAddress.h"
#include "./Socket.h"
#include "./SocketsApi.h"

#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

namespace LibNet
{
	class SSL_ENV
	{
	public:
		SSL_ENV()
		{
#if OPENSSL_VERSION_NUMBER < 0x1010001fL
			SSL_load_error_strings();
			SSL_library_init();
#else
			OPENSSL_init_ssl(OPENSSL_INIT_LOAD_SSL_STRINGS | OPENSSL_INIT_LOAD_CRYPTO_STRINGS, NULL);
#endif
			OpenSSL_add_all_algorithms();
		}
		~SSL_ENV()
		{
#if OPENSSL_VERSION_NUMBER < 0x1010001fL
			ERR_free_strings();
			EVP_cleanup();
#endif
		}
	};

	class SocketSSL : public Socket
	{
	public:
		SocketSSL(IOEventLoop *loop, int fd);
		virtual ~SocketSSL();

		virtual int connect(const InetAddress& peerAddr, bool bBlock = false) override;
		virtual int close() override;
		virtual ssize_t read(char *buf, size_t count, void *param = nullptr) override;
		virtual ssize_t write(const char *buf, size_t count, void *param = nullptr) override;

		virtual int getLastErr()  override;
		virtual string getErrorInfo() override;
		virtual SOCK_STATUS waitOrClose() override;

	private:
		bool init();

		SSL_CTX *ctx;
		SSL* ssl;
		int errSSL;
	};
}
