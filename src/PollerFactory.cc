#include <stdlib.h>
#include "../include/pollerfactory.h"


using namespace LibNet;

IPoller* PollerFactory::getDefaultPoller(IOEventLoop* loop)
{
#if defined (WIN32) || defined(_WIN64)  // windows
    switch (poller_type)
    {
    case POLLERTYPE::IOCP:
        return new IocpPoller(loop);

    default:
        return new SelectPoller(loop);
        break;
    }
#elif defined(MAC_OS)   // mac
    switch (poller_type)
    {
    case POLLERTYPE::KQUEUE:
        return new SelectPoller(loop);

    default:
        return new SelectPoller(loop);
        break;
    }
#else                   // linux
    switch (poller_type)
    {
    case POLLERTYPE::EPOLL:
        return new EPollPoller(loop);

    case POLLERTYPE::POLL:
        return new PollPoller(loop);

    default:
        return new SelectPoller(loop);
        break;
    }

#endif
}
