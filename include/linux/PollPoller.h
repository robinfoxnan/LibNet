#ifndef LIB_NET_POLLER_POLLPOLLER_H
#define LIB_NET_POLLER_POLLPOLLER_H

#include "../IPoller.h"
#include "../Util.h"
#include <vector>
#include <poll.h>

namespace LibNet
{
  using  PollFdList = std::vector<struct pollfd>;
  class IOEventLoop;

  class PollPoller : public IPoller
  {
  public:
    PollPoller(IOEventLoop *loop);
    ~PollPoller() override;

    Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;
    void updateChannel(Channel *channel) override;
    void removeChannel(Channel *channel) override;

  private:
    void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;

    mutable PollFdList pollfds_;
  };

} // namespace LIB_NET
#endif // LIB_NET_POLLER_POLLPOLLER_H
