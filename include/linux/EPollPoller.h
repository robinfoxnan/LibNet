#ifndef LIB_NET_POLLER_EPOLLPOLLER_H
#define LIB_NET_POLLER_EPOLLPOLLER_H

#include "../IPoller.h"
#include "../Util.h"
#include <vector>
#include <sys/epoll.h>

namespace LibNet
{
  // struct epoll_event;
  class IOEventLoop;
  using EventList = std::vector<struct epoll_event>;

  class EPollPoller : public IPoller
  {
  public:
    EPollPoller(IOEventLoop *loop);
    ~EPollPoller() override;

    virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;
    virtual void updateChannel(Channel *channel) override;
    virtual void removeChannel(Channel *channel) override;

  private:
    static const int kInitEventListSize = 16;
    static const char *operationToString(int op);

    void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;
    void update(int operation, Channel *channel);

    int epollfd_;
    EventList events_;
  };

} // namespace LIB_NET
#endif  // LIB_NET_POLLER_EPOLLPOLLER_H
