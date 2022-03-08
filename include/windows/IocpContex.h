#pragma  once
#include "../CommonHeader.h"
#include "../Channel.h"

namespace LibNet
{
	enum IOCP_TYPE
	{
		IOInitialize, // The client just connected
		IORead,       // Read completed
		IOSend,       // Write Completed.
		IOTransmitFileCompleted,
		IODONE,          // check this mask
		IOERROR
	};

	typedef struct IocpContext_
	{
	public:
		IocpContext_(Channel * ch, char *buf, size_t len, IOCP_TYPE cmd) : 
			sock(ch), operation(cmd), ioSize(0)
		{
			memset((char *)&overlapped, 0, sizeof(overlapped));
			wsabuf.buf = buf;
			wsabuf.len = len;
			param = nullptr;
		}

		IocpContext_(Channel * ch, char *buf, size_t len, IOCP_TYPE cmd, void * lp) : 
			sock(ch), operation(cmd), ioSize(0), param(lp)
		{
			memset((char *)&overlapped, 0, sizeof(overlapped));
			wsabuf.buf = buf;
			wsabuf.len = len;
		}

		IocpContext_()
		{
			memset((char *)&overlapped, 0, sizeof(overlapped));
		}

		OVERLAPPED	overlapped;
		Channel *   sock;
		void *      param;
		WSABUF      wsabuf; 
		IOCP_TYPE   operation;

		DWORD       ioSize;   // iocp return it 
	}IocpContext;
}