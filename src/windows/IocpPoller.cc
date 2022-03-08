#include "../../include/CommonLog.h"
#include "../../include/windows/IocpPoller.h"
#include "../../include/windows/IocpContex.h"

using namespace  LibNet;

IocpPoller::IocpPoller(IOEventLoop* loop) : IPoller(loop), iocpHandle(INVALID_HANDLE_VALUE)
{
	iocpHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
	if (INVALID_HANDLE_VALUE == iocpHandle)
	{
		FORMAT_ERROR("iocp init error!!");
	}
}

IocpPoller::~IocpPoller()
{
	if (iocpHandle != INVALID_HANDLE_VALUE)
	{
		::CloseHandle(iocpHandle);
	}
}

void IocpPoller::addActiveChannel(IocpContext * context, ChannelList* activeChannels)
{

	if (context == nullptr)
		return;

	int mask = 0;
	
	
	switch (context->operation)
	{
	case IOCP_TYPE::IORead:
		mask |= Channel::kReadEvent;
		context->operation = IOCP_TYPE::IODONE;
		break;

	case IOCP_TYPE::IOSend:
		mask |= Channel::kWriteEvent;
		context->operation = IOCP_TYPE::IODONE;
		break;

	default:
		mask |= Channel::kErrorEvent;
		context->operation = IOCP_TYPE::IOERROR;
	}

	if (mask)
	{
		assert(context->sock);
		
		Channel * sock = (Channel *)context->sock;
		sock->set_revents(mask);
		sock->handleEvent(0);
	}
		
}

Timestamp IocpPoller::poll(int timeoutMs, ChannelList *activeChannels)
{
	LPOVERLAPPED lpOverlapped;
	Channel * ch = nullptr;

	DWORD dwIoSize;
	BOOL bIORet = GetQueuedCompletionStatus(
		iocpHandle,
		&dwIoSize,
		(PULONG_PTR)&ch,
		&lpOverlapped,
		1); // INFINITE

	if (ch == nullptr)
	{
		return 0;
	}

	// If Something whent wrong..
	if (bIORet == FALSE)
	{
		DWORD dwIOError = GetLastError();
		if (dwIOError != WAIT_TIMEOUT) // It was not an Time out event we wait for ever (INFINITE) 
		{

			if (dwIOError == ERROR_NETNAME_DELETED)
			{
				if (lpOverlapped)
				{
					IocpContext * context = CONTAINING_RECORD(lpOverlapped, IocpContext, overlapped);
					// connection broken
					addActiveChannel(context, activeChannels);
				}	
			}
		}
		else
		{
			// TIMEOUT, try again
			return 0;
		}

	}// error

	if (bIORet && lpOverlapped)
	{
		IocpContext * context = CONTAINING_RECORD(lpOverlapped, IocpContext, overlapped);

		if (context != nullptr)
		{
			context->ioSize = dwIoSize;
			addActiveChannel(context, activeChannels);
		}
	}

	return 0;

}

// so it is thread safe
void IocpPoller::updateChannel(Channel *channel)
{
	std::lock_guard<std::mutex>  guatd(mutexCh);
	socket_t fd = channel->fd();

	auto it = channels_.find(fd);
	if (it == channels_.end())
	{
		HANDLE h = CreateIoCompletionPort((HANDLE)fd, iocpHandle, (ULONG_PTR)channel, 0);
		assert(iocpHandle ==  h);
	}

	int mask = static_cast<short>(channel->events());

	if (mask == Channel::kNoneEvent)
	{
		channels_.erase(fd);
	}
	else
	{
		channels_[fd] = channel;
		//FORMAT_DEBUG("SelectPoller::updateChannel() add to channel %lld", fd);
	}
}

void IocpPoller::removeChannel(Channel *channel)
{
	std::lock_guard<std::mutex>  guatd(mutexCh);
	socket_t fd = channel->fd();
	channels_.erase(fd);
}

