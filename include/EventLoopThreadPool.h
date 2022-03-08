#ifndef LibNet_NET_EVENTLOOPTHREADPOOL_H
#define LibNet_NET_EVENTLOOPTHREADPOOL_H

#include "./CommonHeader.h"

namespace LibNet
{
	class IOEventLoop;
	class EventLoopThreadPool
	{
	public:
		EventLoopThreadPool();
		~EventLoopThreadPool();

		EventLoopThreadPool & Init(uint32_t n);
		void start();
		void stop();
		IOEventLoop* getNextLoop();
		IOEventLoop * getLoopForHash(size_t hashCode);

		static EventLoopThreadPool & instance()
		{
			static EventLoopThreadPool pool;
			return pool;
		}

	private:
		void setThreadId(size_t index, std::thread::id Id);
		void DoWork(size_t  index);
		static void ThreadWorkFun(EventLoopThreadPool * lp, size_t  n);  		// 线程的工作函数
		
	private:

		uint32_t nThreads;

		uint32_t nextLoop;
		std::mutex loopMutex;

		std::vector<std::unique_ptr<IOEventLoop> > vecLoops;
		std::vector<std::thread> vecThreads;  // threads
	   // std::vector<int64_t> vecNums;         // count num

		static std::atomic_bool bStop;

	};

}  // namespace LibNet

#endif  // LibNet_NET_EVENTLOOPTHREADPOOL_H
