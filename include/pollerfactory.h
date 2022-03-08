#pragma once
#include "../include/CommonHeader.h"
#include "../include/IOEventLoop.h"
#include "../include/IPoller.h"
#include "../include/SelectPoller.h"

#if defined (WIN32) || defined(_WIN64)
#include "../include/windows/IocpPoller.h"
#elif defined(MAC_OS)
#include "../include/mac/KqueuePoller.h"
#else  // linux
#include "../include/linux/PollPoller.h"
#include "../include/linux/EPollPoller.h"
#endif

namespace  LibNet {

    class PollerFactory
    {
    public:
         static IPoller *getDefaultPoller(IOEventLoop *loop);
    };

}
