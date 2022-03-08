#pragma once
//TimerHeap.h
#include <functional>
#include <vector>
#include <queue>
#include <mutex>

#include "Util.h"

namespace LibNet
{
	using TimerCallback = std::function<void(int64_t timerId)>;

	// 定时器类
	class HeapTimer
	{
	public:
		HeapTimer(int64_t delayMs, TimerCallback & cb)
		{
			expire = delayMs + Utils::getmSecNow();
			callBackFunc = cb;
			bRepeate = false;
			interval = 0;
		}
		void call()
		{
			if (callBackFunc)
				callBackFunc(timerId);
		}
		int64_t expire;	
		int64_t timerId;
		TimerCallback callBackFunc;
		int interval;
		bool bRepeate;
		//void * data;
	};
	using HeapTimerPtr = std::shared_ptr<HeapTimer> ;

	// 比较函数，实现小根堆
	struct cmp { 
		bool operator () (const HeapTimerPtr &a, const HeapTimerPtr& b) 
		{
			return (a->expire) > (b->expire);
		}
	};

	class TimerHeap {
	public:
		explicit TimerHeap();
		~TimerHeap();

	public:
		int64_t addTimer(int64_t delayMs, int interval, TimerCallback cb);
		int64_t addTimerAt(int64_t tmMark, TimerCallback cb);

		void    delTimer(int64_t id); 
		void    tick();
		const  HeapTimerPtr& top();
		void    removeTop();
		size_t size();

	private:
		std::priority_queue<HeapTimerPtr, std::vector<HeapTimerPtr>, cmp> queTimer; 
		std::mutex mutexId;
		int64_t curId;
	};

}
