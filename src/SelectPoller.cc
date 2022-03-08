#include "../include/SelectPoller.h"
#include "../include/Channel.h"
#include "../include/Util.h"
#include "../include/CommonLog.h"

using namespace LibNet;

SelectPoller::SelectPoller(IOEventLoop* loop) : IPoller(loop)
{
    FD_ZERO(&rfds);
    FD_ZERO(&wfds);
	maxFd = 0;
}

SelectPoller::~SelectPoller() = default;

Timestamp SelectPoller::poll(int timeoutMs, ChannelList* activeChannels)
{
    int retval, numevents = 0;
    timeval tv;
    tv.tv_sec = 0;//
    tv.tv_usec = 0;// windows should set it to be 0, or will be slow

    memcpy(&_rfds, &rfds, sizeof(fd_set));
    memcpy(&_wfds, &wfds, sizeof(fd_set));
	memcpy(&_efds, &efds, sizeof(fd_set));
	
	//int n = efds.fd_count;
		//n =max(_rfds.fd_count, _wfds.fd_count);
	//if (n==0)
	//	return Utils::getmSecNow();

	//printf("select n %d \n", n);
    retval = select(maxFd + 1, &_rfds, &_wfds, &_efds, &tv);

	//printf("select return %d \n", retval);
    if (retval > 0)  // SOCKET_ERROR
	{
		fillActiveChannels(retval, activeChannels);
	}
    else
    {
        if (retval == -1)
        {
            FORMAT_ERROR("select error");
        }
    }
    
    Timestamp now = Utils::getmSecNow();
    return now;

}

void SelectPoller::fillActiveChannels(int numEvents, ChannelList* activeChannels) const
{
    activeChannels->clear();
    if (numEvents > 0) 
    {
		int mask = 0;
        for (auto it : channels_) 
        {
            mask = 0;
            socket_t fd = it.first;
            Channel * ch = it.second;

            if (ch->events() & Channel::kReadEvent && FD_ISSET(fd, &_rfds))
                mask = Channel::kReadEvent;

            if (ch->events() & Channel::kWriteEvent && FD_ISSET(fd, &_wfds))
                mask |= Channel::kWriteEvent;

			if (FD_ISSET(fd, &_efds))
			{
				mask |= Channel::kErrorEvent;
			}

            if (mask)
            {
                ch->set_revents(mask);
                activeChannels->emplace_back(ch);
            }
            
        }
    }
}

void SelectPoller::updateChannel(Channel* channel)
{
 
    // a new one, add to pollfds_
    //assert(channels_.find(channel->fd()) == channels_.end());

	
    socket_t fd = channel->fd();

	if (fd > maxFd)
		maxFd = fd;

    int mask = static_cast<short>(channel->events());

	// read
	if (mask & Channel::kReadEvent)
	{
		FD_SET(fd, &rfds);
	}
	else
	{
		FD_CLR(fd, &rfds);
	}

	// write 
	if (mask & Channel::kWriteEvent)
	{
		FD_SET(fd, &wfds);
	}
	else
	{
		FD_CLR(fd, &wfds);
	}


	if (mask == Channel::kNoneEvent)
	{
		FD_CLR(fd, &efds);
		channels_.erase(fd);
	}
	else
	{
		FD_SET(fd, &efds);
		channels_[fd] = channel;
		FORMAT_DEBUG("SelectPoller::updateChannel() add to channel %lld", fd);
	}
}

void SelectPoller::removeChannel(Channel* channel)
{
    socket_t fd = channel->fd();
    int mask = static_cast<short>(channel->events());

    // FD_CLR windows find it in array, and remove it
	//        linux set mask in array
    FD_CLR(fd, &rfds);
    FD_CLR(fd, &wfds);
	FD_CLR(fd, &efds);

	auto it = channels_.find(fd);
	if (it == channels_.end())
	{

	}
	else
	{
		channels_.erase(fd);
	}
}

