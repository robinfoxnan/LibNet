#include "../include/SocketSSL.h"
#include "../include/CommonLog.h"

using namespace LibNet;

static SSL_ENV ssl_init_instance;

SocketSSL::SocketSSL(IOEventLoop *loop, int fd): Socket(loop, fd),ssl(nullptr), ctx(nullptr)
{
	bool ret = init();
	(void)ret;
	FORMAT_DEBUG("init ssl ret = %d", (int)ret);
}
SocketSSL::~SocketSSL()
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

bool SocketSSL::init()
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

int SocketSSL::connect(const InetAddress& peerAddr, bool bBlock)
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
		SSL_set_fd(ssl, (int)fd_);
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

int SocketSSL::close()
{
	if (ssl)
	{
		SSL_shutdown(ssl);
		SSL_clear(ssl);
	}
	return Socket::close();
}

ssize_t SocketSSL::read(char *buf, size_t count, void *param)
{
	int n = SSL_read(ssl, buf, (int)count);
	this->errSSL = SSL_get_error(ssl, n);
	return (ssize_t)n;

}

ssize_t SocketSSL::write(const char *buf, size_t count, void *param)
{
	int n = SSL_write(ssl, buf, (int)count);
	this->errSSL = SSL_get_error(ssl, n);
	return (ssize_t)n;
}

int SocketSSL::getLastErr()
{
	return errSSL;
}
string SocketSSL::getErrorInfo()
{
	return ERR_error_string(errSSL, NULL);
}

SOCK_STATUS SocketSSL::waitOrClose()
{
	switch (errSSL)
	{
	case SSL_ERROR_NONE:
		return SOCK_OK;

	case SSL_ERROR_WANT_READ:   // 2
	case SSL_ERROR_WANT_WRITE:  // 3
		return SOCK_STATUS::SOCK_RETRY;

	default:                  
		return SOCK_STATUS::SOCK_ERROR;
	}

}


