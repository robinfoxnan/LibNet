#ifndef LibNet_NET_POLLER_H
#define LibNet_NET_POLLER_H

#include "CommonHeader.h"

namespace LibNet
{

  class Channel;
  class IOEventLoop;
  //using ChannelPtr = std::weak_ptr<Channel>;
  typedef std::map<socket_t, Channel *> ChannelMap;
  typedef std::vector<Channel *> ChannelList;


  class IPoller
  {
  public:
    IPoller(IOEventLoop *loop) : ownerLoop_(loop)
    {
    }
    virtual ~IPoller() = default;


    virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels)
    {
        return 0;
    }

    virtual void updateChannel(Channel *channel) {}
    virtual void removeChannel(Channel *channel) {}
	virtual bool isThreadSafe() { return false; }

    bool hasChannel(Channel *channel);
    void assertInLoopThread() const;

  protected:
	std::mutex mutexCh;
    ChannelMap channels_;

  private:
    IOEventLoop *ownerLoop_;
  };

} // namespace LibNet

#endif // LibNet_NET_POLLER_H
