#ifndef  __WIN_SOCKET_INIT
#define  __WIN_SOCKET_INIT

#include "../CommonHeader.h"
namespace LibNet
{
	class WSInit 
	{
	public:
		WSInit() 
		{
			WSADATA wsaData;
			WSAStartup(MAKEWORD(2, 2), &wsaData);
		}

		~WSInit() { WSACleanup(); }
	};

}// end space name

#endif // __WIN_SOCKET_INIT


