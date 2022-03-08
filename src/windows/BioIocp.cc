/* 代码来自：
** <<一种 Windows IOCP 整合 OpenSSL 实现方案>>
**
** by 阙荣文 Que's C++ Studio
** 2021-06-06
*/

#include "../../include/windows/BioIocp.h"

using namespace LibNet;

static int bio_iocp_write(BIO *h, const char *buf, int num);
static int bio_iocp_read(BIO *h, char *buf, int size);
static int bio_iocp_puts(BIO *h, const char *str);
static long bio_iocp_ctrl(BIO *h, int cmd, long arg1, void *arg2);
static int bio_iocp_create(BIO *h);
static int bio_iocp_destroy(BIO *data);

//* 新版 openssl 库不再暴露 BIO_METHOD 结构,所以不能用下列语句直接声明一个 BIO_METHOD 结构
// 必须通过 BIO_meth_xxx() 接口访问
// 
//static BIO_METHOD methods_iosockp = 
//{
//	BIO_TYPE_SOCKET,
//	"iocp_socket",
//	bio_iocp_write,
//	bio_iocp_read,
//	bio_iocp_puts,
//	NULL,                       /* sock_gets, */
//	bio_iocp_ctrl,
//	bio_iocp_create,
//	bio_iocp_destroy,
//	NULL,
//	0
//};

int inst_index = -1;

///*
//* 没找到如何实现自定义 BIO 文档说明
//* 问题1: 哪些 bio_method 需要实现,哪些可选?
//* 问题2: 各自定义函数的参数,返回值等有何规范?
//* 
//* 参考 bio_mem 的实现 bio/bss_mem.c
// 
//* 实践测试,下列说明的三个方法必须实现 read/write/ctrl
//* 调试过程发现 调用了 6, 11, 11, 7 调用 ctrl, 即要保证 BIO_flush() 调用成功
//# define BIO_CTRL_RESET          1/* opt - rewind/zero etc */
//# define BIO_CTRL_EOF            2/* opt - are we at the eof */
//# define BIO_CTRL_INFO           3/* opt - extra tit-bits */
//# define BIO_CTRL_SET            4/* man - set the 'IO' type */
//# define BIO_CTRL_GET            5/* man - get the 'IO' type */
//# define BIO_CTRL_PUSH           6/* opt - internal, used to signify change */
//# define BIO_CTRL_POP            7/* opt - internal, used to signify change */
//# define BIO_CTRL_GET_CLOSE      8/* man - set the 'close' on free */
//# define BIO_CTRL_SET_CLOSE      9/* man - set the 'close' on free */
//# define BIO_CTRL_PENDING        10/* opt - is their more data buffered */
//# define BIO_CTRL_FLUSH          11/* opt - 'flush' buffered output */
//# define BIO_CTRL_DUP            12/* man - extra stuff for 'duped' BIO */
//# define BIO_CTRL_WPENDING       13/* opt - number of bytes still to write */
//# define BIO_CTRL_SET_CALLBACK   14/* opt - set callback function */
//# define BIO_CTRL_GET_CALLBACK   15/* opt - set callback function */
// 
// 
//* 备用解决方案: 使用 bio_mem 而不自定义 bio, 再从 bio_mem 中读取后投递 iocp 请求
//*/

// SSL_Free() will free resource
BIO * BioIocp::newBIO(SocketSSLIocp * data)
{
	BIO_METHOD * method = BIO_meth_new(BIO_get_new_index() | BIO_TYPE_SOURCE_SINK, "bio_iocp");
	//BIO_meth_set_create(method, bio_iocp_create);/* 可选 */
	//BIO_meth_set_destroy(method, bio_iocp_destroy);/* 可选 */
	BIO_meth_set_ctrl(method, bio_iocp_ctrl); /* 自定义 BIO 必须实现 */
	BIO_meth_set_read(method, bio_iocp_read);/* 自定义 BIO 必须实现 */
	//BIO_meth_set_puts(method, bio_iocp_puts); /* 可选 */
	BIO_meth_set_write(method, bio_iocp_write);/* 自定义 BIO 必须实现 */

	BIO* bio = BIO_new(method);
	BIO_set_data(bio, data);
	BIO_set_init(bio, 1);
	BIO_set_shutdown(bio, 0);
	return bio;
}

static int bio_iocp_create(BIO *bi)
{
	return 1;
}

static int bio_iocp_destroy(BIO *a)
{
	return 1;
}

static long bio_iocp_ctrl(BIO* b, int cmd, long num, void* ptr)
{
	//sgtrace("bio_iocp_ctrl(%d, %ld, %p)\n", cmd, num, ptr);

	int ret = 0;
	switch (cmd)
	{
	case BIO_CTRL_FLUSH:
	{
		ret = 1;
		break;
	}
	default:
		ret = 0;
	}
	return ret;
}

// 返回值: >0 实际读取到的数据长度; <=0 发生错误,由 bio_flags 指定原因
static int bio_iocp_read(BIO *bio, char *out, int outl)
{
	SocketSSLIocp *sock = (SocketSSLIocp *)BIO_get_data(bio);
	assert(sock);
	int ret = 0;

	if (out != NULL)
	{
		ret = sock->recvRaw((char*)out, (size_t)outl);
		BIO_clear_retry_flags(bio);
		if (ret <= 0)
		{
			sock->getLastErr();
			SOCK_STATUS code = sock->waitOrClose();
			if (code == SOCK_STATUS::SOCK_WAIT)
			{
				BIO_set_retry_read(bio);
			}
			if (code == SOCK_STATUS::SOCK_ERROR)
			{
				//BIO_set_shutdown(bio, 1);
			}
		}
	}
	return ret;
}

static int bio_iocp_write(BIO *bio, const char *buf, int len)
{
	SocketSSLIocp *sock = (SocketSSLIocp *)BIO_get_data(bio);
	assert(sock);

	int ret = sock->sendRaw(buf, len);
	BIO_clear_retry_flags(bio);
	if (ret <= 0)
	{
		sock->getLastErr();
		SOCK_STATUS code = sock->waitOrClose();
		if (code == SOCK_STATUS::SOCK_WAIT)
		{
			BIO_set_retry_read(bio);
		}
		if (code == SOCK_STATUS::SOCK_ERROR)
		{
			//BIO_set_shutdown(bio, 1);
			
		}
	}
	return ret;
}

static int bio_iocp_puts(BIO *bio, const char *str)
{
	return bio_iocp_write(bio, str, strlen(str));
}
