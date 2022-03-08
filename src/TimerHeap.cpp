//TimerHeap.cpp
#include "../include/TimerHeap.h"

using namespace  LibNet;
TimerHeap::TimerHeap():curId(1)
{

}

TimerHeap::~TimerHeap() 
{
	while (!queTimer.empty())
	{
		queTimer.pop();
	}
}

int64_t TimerHeap::addTimerAt(int64_t tmMark, TimerCallback cb)
{
	if (cb == nullptr)
		return -1;

	int64_t tm = Utils::getmSecNow();
	if (tm >= tmMark)
	{
		// user should run callback  himself
		return -1;
	}

	HeapTimerPtr timer = std::make_shared<HeapTimer>(0, cb);
	timer->expire = tm;

	std::lock_guard<std::mutex> guard(mutexId);
	timer->timerId = curId;
	curId++;
	queTimer.push(timer);

	return timer->timerId;
}

int64_t TimerHeap::addTimer(int64_t delayMs, int interval, TimerCallback cb)
{
	if (cb == nullptr)
		return -1;

	HeapTimerPtr timer = std::make_shared<HeapTimer>(delayMs, cb);
	if (interval > 0)
	{
		timer->interval = interval;
		timer->bRepeate = true;
	}

	std::lock_guard<std::mutex> guard(mutexId);
	timer->timerId = curId;
	curId++;
	queTimer.push(timer);

	return timer->timerId;
}

void  TimerHeap::delTimer(int64_t id)
{
	std::lock_guard<std::mutex> guard(mutexId);

	std::deque<HeapTimerPtr> tempQue;
	while (!queTimer.empty())
	{
		HeapTimerPtr ptr = queTimer.top();
		queTimer.pop();
		if (ptr->timerId != id)
			tempQue.push_back(ptr);
	}

	// push back tempQue
	for (size_t i = 0; i < tempQue.size(); i++)
	{
		queTimer.push(tempQue[i]);
	}
	tempQue.clear();
}
void TimerHeap::tick()
{
	std::lock_guard<std::mutex> guard(mutexId);

	if (queTimer.empty())
		return;

	std::deque<HeapTimerPtr> tempQue;
	int64_t  tm = Utils::getmSecNow();
	if (queTimer.top()->expire <= tm)
	{
		while (!queTimer.empty())
		{
			HeapTimerPtr timer = queTimer.top();
			if (timer->expire <= tm)
			{
				//DEBUG_PRINT("tm=%lld, expire=%lld, delta=%lld \n", tm, timer->expire, timer->timerId);
				timer->call();
				if (timer->bRepeate)
				{
					timer->expire = timer->interval + tm;
					tempQue.emplace_back(timer);
				}
				queTimer.pop();
			}
			else
			{
				break;
			}
			
		}

		// push back tempQue
		for (size_t i=0; i<tempQue.size(); i++)
		{
			queTimer.push(tempQue[i]);
		}
		tempQue.clear();

	}
}

const HeapTimerPtr& TimerHeap::top()
{
	std::lock_guard<std::mutex> guard(mutexId);
	return queTimer.top();
}

void    TimerHeap::removeTop()
{
	std::lock_guard<std::mutex> guard(mutexId);
	queTimer.pop();
}
size_t TimerHeap::size()
{
	std::lock_guard<std::mutex> guard(mutexId);
	return queTimer.size();
}



