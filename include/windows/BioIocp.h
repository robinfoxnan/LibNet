#pragma once
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

#include "SocketSSLIocp.h"

namespace LibNet
{
	class BioIocp
	{
	public:
		static BIO * newBIO(SocketSSLIocp * data);
	};
}


