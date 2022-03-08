#include "../../include/windows/SocketSSLIocp.h"
#include "../../include/CommonLog.h"
#include "../../include/windows/BioIocp.h"

using namespace LibNet;


SocketSSLIocp::SocketSSLIocp(IOEventLoop *loop, int fd): SocketIocp(loop, fd),ssl(nullptr), ctx(nullptr)
{
	bool ret = init();
	(void)ret;
	FORMAT_DEBUG("init ssl ret = %d", (int)ret);
}
SocketSSLIocp::~SocketSSLIocp()
{
	if (ssl)
	{
		SSL_shutdown(ssl);
		SSL_clear(ssl);
		SSL_free(ssl);
		ssl = nullptr;
	}
	if (ctx)
	{
		SSL_CTX_free(ctx);
		ctx = nullptr;
	}
}

bool SocketSSLIocp::init()
{
	const SSL_METHOD *meth = SSLv23_client_method();
	ctx = SSL_CTX_new(meth);
	if (ctx)
	{
		SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);
		SSL_CTX_set_verify_depth(ctx, 0);
		SSL_CTX_set_mode(ctx, SSL_MODE_AUTO_RETRY);
		SSL_CTX_set_session_cache_mode(ctx, SSL_SESS_CACHE_CLIENT);
	}

	return ctx;
}

int SocketSSLIocp::connect(const InetAddress& peerAddr, bool bBlock)
{
	// only use syn mode;
	bBlock = true;
	int ret = Socket::connect(peerAddr, bBlock);
	if (ret != 0)
	{
		FORMAT_DEBUG("connect error, didn't ssl_connect");
		return ret;
	}
	if (ssl == nullptr)
	{
		this->ssl = SSL_new(ctx);
		
		BIO* bio = BioIocp::newBIO(this);
		SSL_set_bio(ssl, bio, bio);
	}

	//SSL_set_tlsext_host_name(ssl, host);
	ret = SSL_connect(ssl);
	errSSL = SSL_get_error(ssl, ret);
	if (ret != 1) 
	{
		return -1;
	}
	return 0;
}

int SocketSSLIocp::close()
{
	if (ssl)
	{
		SSL_shutdown(ssl);
		SSL_clear(ssl);
	}
	return Socket::close();
}

ssize_t SocketSSLIocp::read(char *buf, size_t count, void *param)
{
	int n = SSL_read(ssl, buf, (int)count);
	this->errSSL = SSL_get_error(ssl, n);
	return (ssize_t)n;

}

ssize_t SocketSSLIocp::write(const char *buf, size_t count, void *param)
{
	int n = SSL_write(ssl, buf, (int)count);
	this->errSSL = SSL_get_error(ssl, n);
	return (ssize_t)n;
}

int SocketSSLIocp::getLastErr()
{
	return errSSL;
}
string SocketSSLIocp::getErrorInfo()
{
	return ERR_error_string(errSSL, NULL);
}

SOCK_STATUS SocketSSLIocp::waitOrClose()
{
	switch (errSSL)
	{
	case SSL_ERROR_NONE:
		return SOCK_OK;

	case SSL_ERROR_WANT_READ:   // 2
	case SSL_ERROR_WANT_WRITE:  // 3
		return SOCK_STATUS::SOCK_RETRY;

	case SSL_ERROR_ZERO_RETURN:
	default:                  
		return SOCK_STATUS::SOCK_ERROR;
	}

}

// used by BIO
int SocketSSLIocp::recvRaw(char * buf, size_t len)
{
	return SocketIocp::read(buf, len);
}

int SocketSSLIocp::sendRaw(const char * buf, size_t len)
{
	return SocketIocp::write(buf, len);
}


