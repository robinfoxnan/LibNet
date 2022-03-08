#pragma once

#if defined(_WIN32) || defined(_WIN64)
#include "../include/windows/SocketInit.h"
#endif

#include "../include/IPoller.h"
#include "../include/CommonHeader.h"


#undef FD_SETSIZE   // 64
#define FD_SETSIZE 1024

#undef  MAXIMUM_WAIT_OBJECTS    // 64
#define MAXIMUM_WAIT_OBJECTS    1024     //要等待的对象数

namespace LibNet
{
  class IOEventLoop;

  class SelectPoller : public IPoller
  {
  public:
    SelectPoller(IOEventLoop *loop);
    ~SelectPoller() override;

	virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;
	virtual void updateChannel(Channel *channel) override;
	virtual void removeChannel(Channel *channel) override;

  private:
    void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;

	// just like redis
    fd_set rfds, wfds, efds;
    /* We need to have a copy of the fd sets as it's not safe to reuse
     * FD sets after select(). */
    fd_set _rfds, _wfds, _efds;

	int maxFd;
  };

} // namespace LibNet

