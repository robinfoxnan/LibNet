#include "../include/IPoller.h"
#include "../include/Channel.h"
#include "../include/IOEventLoop.h"

using namespace  LibNet;

bool IPoller::hasChannel(Channel *channel)
    {
      //assertInLoopThread();
      auto it = channels_.find(channel->fd());
      return it != channels_.end() && it->second == channel;
    }


void IPoller::assertInLoopThread() const
{
  ownerLoop_->assertInLoopThread();
}
