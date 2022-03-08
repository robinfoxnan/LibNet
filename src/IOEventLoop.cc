#include "../include/IOEventLoop.h"

#include "../include/Channel.h"
#include "../include/IPoller.h"
#include "../include/SocketsApi.h"
#include "../include/pollerfactory.h"

#include "../include/CommonHeader.h"
#include <signal.h>



using namespace LibNet;

namespace LibNet
{
	thread_local IOEventLoop *t_loopInThisThread = nullptr;

	const int kPollTimeMs = 10;

	/* int createEventfd()
	 {
	   int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
	   if (evtfd < 0)
	   {
		 LOG_SYSERR << "Failed in eventfd";
		 abort();
	   }
	   return evtfd;
	 }*/


} // namespace

// make get pointer easy
IOEventLoop *IOEventLoop::getIOEventLoopOfCurrentThread()
{
	return t_loopInThisThread;
}
//////////////////////////////////////////////////////////////////////////

IOEventLoop::IOEventLoop() :
	poller(PollerFactory::getDefaultPoller(this)),
	currentActiveChannel(nullptr)
{

	if (t_loopInThisThread)
	{

	}
	else
	{
		t_loopInThisThread = this;
	}

	TimerHeap *heap = new TimerHeap();
	this->timerHeap = std::unique_ptr<TimerHeap>(heap);

}

IOEventLoop::~IOEventLoop()
{
	t_loopInThisThread = nullptr; 
	this->timerHeap = nullptr;
}

void IOEventLoop::loopOnce()
{
	assertInLoopThread();

	activeChannels.clear();
	pollReturnTime = poller->poll(kPollTimeMs, &activeChannels);


	// poll state from sockets alike
	bEventHandling = true;
	for (Channel *channel : activeChannels)
	{
		currentActiveChannel = channel;
		currentActiveChannel->handleEvent(pollReturnTime);
	}
	currentActiveChannel = NULL;
	bEventHandling = false;

	// call functors
	doPendingFunctors();

	// call timer
	if (timerHeap)
	{
		timerHeap->tick();
	}

}
int64_t IOEventLoop::addTimer(int64_t delayMs, int interval, TimerCallback cb)
{
	return timerHeap->addTimer(delayMs, interval, cb);
}

void  IOEventLoop::delTimer(int64_t id)
{
	return timerHeap->delTimer(id);
}

void IOEventLoop::runInLoop(Channel *ch, Functor cb)
{
	if (isInLoopThread())
	{
		//printf("is in the loop\n");
		cb();
	}
	else
	{
		//printf("not in the loop\n");
		queueInLoop(ch, cb);
	}
}

void IOEventLoop::queueInLoop(Channel *ch, Functor cb)
{
	{
		std::lock_guard<std::mutex> guard(mutexQue);
		functorQue.emplace_back(std::make_pair(ch, std::move(cb)));
	}

	if (!isInLoopThread() || bCallingPendingFunctors)
	{
		//wakeup();
	}
}

size_t IOEventLoop::queueSize()
{
	std::lock_guard<std::mutex> guard(mutexQue);
	return functorQue.size();
}

// iocpPoller is not in loop, it's thread safe
// 
void IOEventLoop::updateChannel(Channel *channel)
{
	assert(channel->ownerLoop() == this);
	
	//assertInLoopThread();
	poller->updateChannel(channel);

}

void IOEventLoop::removeChannel(Channel *channel)
{
	assert(channel->ownerLoop() == this);
	assertInLoopThread();
	if (bEventHandling)
	{
		//assert(currentActiveChannel_ == channel ||
		//       std::find(activeChannels_.begin(), activeChannels_.end(), channel) == activeChannels_.end());
	}
	poller->removeChannel(channel);

	// remove functors form queue
	std::lock_guard<std::mutex> guard(mutexQue);
	{
		for (auto it = functorQue.begin(); it != functorQue.end(); )
		{
			if ((*it).first == channel)
			{
				it = functorQue.erase(it);
			}
			else
			{
				it++;
			}
		}
	}
}

bool IOEventLoop::hasChannel(Channel *channel)
{
	assert(channel->ownerLoop() == this);
	//assertInLoopThread();
	return poller->hasChannel(channel);
}

void IOEventLoop::abortNotInLoopThread()
{
	std::cout << "IOEventLoop::abortNotInLoopThread() IOEventLoop " << this
		<< " was created in threadId_ = " << threadId
		<< ", current thread id = " << std::this_thread::get_id() << std::endl;
}



void IOEventLoop::handleRead()
{
	uint64_t one = 1;
	//  ssize_t n = socketsApi::read(wakeupFd_, &one, sizeof one);
	//  if (n != sizeof one)
	//  {
	//    LOG_ERROR << "IOEventLoop::handleRead() reads " << n << " bytes instead of 8";
	//  }
}

void IOEventLoop::doPendingFunctors()
{
	bCallingPendingFunctors = true;
	std::deque<Job> functors;
	{
		std::lock_guard<std::mutex> guard(this->mutexQue);
		functors.swap(this->functorQue);
	}

	for (const Job & job : functors)
	{
		// call function
		job.second();
	}
	bCallingPendingFunctors = false;
}

void IOEventLoop::printActiveChannels() const
{
	for (const Channel *channel : activeChannels)
	{
		// LOG_TRACE << "{" << channel->reventsToString() << "} ";
	}
}


