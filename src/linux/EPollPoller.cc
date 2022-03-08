#include <assert.h>
#include <errno.h>

#include <poll.h>
#include <unistd.h>

#include "../../include/linux/EPollPoller.h"
#include "../../include/Channel.h"
#include "../../include/CommonLog.h"

using namespace LibNet;

// On Linux, the constants of poll(2) and epoll(4)
// are expected to be the same.
static_assert(EPOLLIN == POLLIN, "epoll uses same flag values as poll");
static_assert(EPOLLPRI == POLLPRI, "epoll uses same flag values as poll");
static_assert(EPOLLOUT == POLLOUT, "epoll uses same flag values as poll");
static_assert(EPOLLRDHUP == POLLRDHUP, "epoll uses same flag values as poll");
static_assert(EPOLLERR == POLLERR, "epoll uses same flag values as poll");
static_assert(EPOLLHUP == POLLHUP, "epoll uses same flag values as poll");

namespace
{
  const int kNew = -1;
  const int kAdded = 1;
  const int kDeleted = 2;
}

EPollPoller::EPollPoller(IOEventLoop *loop) : IPoller(loop),
                                              
                                              events_(kInitEventListSize)
{
  epollfd_ = ::epoll_create1(EPOLL_CLOEXEC);
  if (epollfd_ < 0)
  {
      FORMAT_ERROR("create epoll fd error");
  }
}

EPollPoller::~EPollPoller()
{
  ::close(epollfd_);
}

Timestamp EPollPoller::poll(int timeoutMs, ChannelList *activeChannels)
{
  FORMAT_DEBUG("EPollPoller::poll fd total count %zu", channels_.size());
  int numEvents = ::epoll_wait(epollfd_,
                               (epoll_event *)(&*events_.begin()),
                               static_cast<int>(events_.size()),
                               timeoutMs);
  int savedErrno = errno;
  Timestamp now = Utils::getmSecNow();
  FORMAT_DEBUG("events happened = %d", numEvents);
  if (numEvents > 0)
  {
    
    fillActiveChannels(numEvents, activeChannels);
    if (static_cast<size_t>(numEvents) == events_.size())
    {
      events_.resize(events_.size() * 2);
    }
  }
  else if (numEvents == 0)
  {

  }
  else
  {
    // error happens, log uncommon ones
    if (savedErrno != EINTR)
    {
      errno = savedErrno;
      FORMAT_ERROR("epoll_wait error = %d", savedErrno);;
    }
  }
  return now;
}

void EPollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const
{
  assert(static_cast<size_t>(numEvents) <= events_.size());
  for (int i = 0; i < numEvents; ++i)
  {
    Channel *channel = static_cast<Channel *>(events_[i].data.ptr);
#ifndef NDEBUG
    int fd = channel->fd();
    ChannelMap::const_iterator it = channels_.find(fd);
    assert(it != channels_.end());
    assert(it->second == channel);
#endif
    channel->set_revents(events_[i].events);
    activeChannels->push_back(channel);
  }
}

void EPollPoller::updateChannel(Channel *channel)
{
  IPoller::assertInLoopThread();
  const int index = channel->index();
  //LOG_TRACE << "fd = " << channel->fd() << " events = " << channel->events() << " index = " << index;
  if (index == kNew || index == kDeleted)
  {
    // a new one, add with EPOLL_CTL_ADD
    int fd = channel->fd();
    if (index == kNew)
    {
      assert(channels_.find(fd) == channels_.end());
      channels_[fd] = channel;
    }
    else // index == kDeleted
    {
      assert(channels_.find(fd) != channels_.end());
      assert(channels_[fd] == channel);
    }

    channel->set_index(kAdded);
    update(EPOLL_CTL_ADD, channel);
  }
  else
  {
    // update existing one with EPOLL_CTL_MOD/DEL
    int fd = channel->fd();
    (void)fd;
    assert(channels_.find(fd) != channels_.end());
    assert(channels_[fd] == channel);
    assert(index == kAdded);
    if (channel->isNoneEvent())
    {
      update(EPOLL_CTL_DEL, channel);
      channel->set_index(kDeleted);
    }
    else
    {
      update(EPOLL_CTL_MOD, channel);
    }
  }
}

void EPollPoller::removeChannel(Channel *channel)
{
  IPoller::assertInLoopThread();
  int fd = channel->fd();
  FORMAT_DEBUG("remove fd=%d", fd);
  assert(channels_.find(fd) != channels_.end());
  assert(channels_[fd] == channel);
  assert(channel->isNoneEvent());
  int index = channel->index();
  assert(index == kAdded || index == kDeleted);
  size_t n = channels_.erase(fd);
  (void)n;
  assert(n == 1);

  if (index == kAdded)
  {
    update(EPOLL_CTL_DEL, channel);
  }
  channel->set_index(kNew);
}

void EPollPoller::update(int operation, Channel *channel)
{
  struct epoll_event event;
  memset(&event, 0, sizeof(event));
  event.events = channel->events();
  event.data.ptr = channel;
  int fd = channel->fd();
 
  FORMAT_DEBUG("epoll_ctl op = %s, fd=%d, event = { %s }", 
  operationToString(operation), fd, channel->eventsToString().c_str());
  if (::epoll_ctl(epollfd_, operation, fd, &event) < 0)
  {
    // ENOMEM
    // ENOSPC
    FORMAT_ERROR("epoll_ctl error!!!");
  }
}

const char *EPollPoller::operationToString(int op)
{
  switch (op)
  {
  case EPOLL_CTL_ADD:
    return "ADD";
  case EPOLL_CTL_DEL:
    return "DEL";
  case EPOLL_CTL_MOD:
    return "MOD";
  default:
    assert(false && "ERROR op");
    return "Unknown Operation";
  }
}
