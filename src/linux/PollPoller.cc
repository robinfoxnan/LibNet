#include <assert.h>
#include <errno.h>
#include <poll.h>

#include "../../include/linux/PollPoller.h"
#include "../../include/Channel.h"
#include "../../include/CommonLog.h"

using namespace LibNet;

PollPoller::PollPoller(IOEventLoop *loop) : IPoller(loop)
{
}

PollPoller::~PollPoller() = default;

Timestamp PollPoller::poll(int timeoutMs, ChannelList *activeChannels)
{
  // XXX pollfds_ shouldn't change
  int numEvents = ::poll(static_cast<struct pollfd *>(&*pollfds_.begin()), pollfds_.size(), timeoutMs);
  int savedErrno = errno;
  Timestamp now = Utils::getmSecNow();

    FORMAT_DEBUG(" events happened = %d",  numEvents);
  if (numEvents > 0)
  {
    fillActiveChannels(numEvents, activeChannels);
  }
  else if (numEvents == 0)
  {
  }
  else
  {
    if (savedErrno != EINTR)
    {
      errno = savedErrno;
      FORMAT_ERROR("poll() error = %d", savedErrno);
    }
  }
  return now;
}

void PollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const
{
  // PollFdList::const_iterator
  for (PollFdList::iterator  pfd = pollfds_.begin();
       pfd != pollfds_.end() && numEvents > 0; 
       ++pfd)
  {
    if (pfd->revents > 0)
    {
      --numEvents;
      ChannelMap::const_iterator ch = channels_.find(pfd->fd);
      assert(ch != channels_.end());
      Channel *channel = ch->second;
      assert(channel->fd() == pfd->fd);
      channel->set_revents(pfd->revents);
      // ?needed?
      pfd->revents = (short)0;
      activeChannels->push_back(channel);
    }
  }
}

void PollPoller::updateChannel(Channel *channel)
{
  IPoller::assertInLoopThread();
  FORMAT_DEBUG("update channel fd=%d, event={%s}", channel->fd(), channel->eventsToString().c_str());
  if (channel->index() < 0)
  {
    // a new one, add to pollfds_
    assert(channels_.find(channel->fd()) == channels_.end());
    struct pollfd pfd;
    pfd.fd = channel->fd();
    pfd.events = static_cast<short>(channel->events());
    pfd.revents = 0;
    pollfds_.push_back(pfd);
    int idx = static_cast<int>(pollfds_.size()) - 1;
    channel->set_index(idx);
    channels_[pfd.fd] = channel;
  }
  else
  {
    // update existing one
    assert(channels_.find(channel->fd()) != channels_.end());
    assert(channels_[channel->fd()] == channel);
    int idx = channel->index();
    assert(0 <= idx && idx < static_cast<int>(pollfds_.size()));
    struct pollfd &pfd = pollfds_[idx];
    assert(pfd.fd == channel->fd() || pfd.fd == -channel->fd() - 1);
    pfd.fd = channel->fd();
    pfd.events = static_cast<short>(channel->events());
    pfd.revents = 0;
    if (channel->isNoneEvent())
    {
      // ignore this pollfd
      pfd.fd = -channel->fd() - 1;
    }
  }
}

void PollPoller::removeChannel(Channel *channel)
{
  IPoller::assertInLoopThread();

  FORMAT_DEBUG("remove channel fd=%d", channel->fd());
  assert(channels_.find(channel->fd()) != channels_.end());
  assert(channels_[channel->fd()] == channel);
  assert(channel->isNoneEvent());

  int idx = channel->index();
  assert(0 <= idx && idx < static_cast<int>(pollfds_.size()));
  const struct pollfd &pfd = pollfds_[idx];

  (void)pfd;
  //assert(pfd.fd == -channel->fd() - 1 && pfd.events == channel->events());
  size_t n = channels_.erase(channel->fd());
  assert(n == 1);
  (void)n;

  if (static_cast<size_t>(idx) == pollfds_.size() - 1)
  {
    pollfds_.pop_back();
  }
  else
  {
    int channelAtEnd = pollfds_.back().fd;
    // not move all the rest, but only swap idx and the last one!
    iter_swap(pollfds_.begin() + idx, pollfds_.end() - 1);
    if (channelAtEnd < 0)
    {
      channelAtEnd = -channelAtEnd - 1;
    }
    channels_[channelAtEnd]->set_index(idx);
    pollfds_.pop_back();
  }
  // robin add
  channel->set_index(-1);
}
