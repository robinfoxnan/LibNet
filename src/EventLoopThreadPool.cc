#include "../include/IOEventLoop.h"
#include "../include/EventLoopThreadPool.h"


using namespace LibNet;
std::atomic_bool EventLoopThreadPool::bStop = {false};

EventLoopThreadPool::EventLoopThreadPool() : nThreads(1), nextLoop(0)
{
}

EventLoopThreadPool::~EventLoopThreadPool()
{
	stop();
}

EventLoopThreadPool & EventLoopThreadPool::Init(uint32_t n)
{
    nThreads = n;
    if (nThreads < 1)
        nThreads = 1;
	return *this;
}

void EventLoopThreadPool::start()
{
    if (vecThreads.size() > 0)
        return;

    nextLoop = 0;

    bStop = false;
    for (uint32_t i = 0; i < nThreads; i++)
    {
        IOEventLoop * p = new IOEventLoop();
        std::unique_ptr<IOEventLoop> ptr(p);

        vecLoops.emplace_back(std::move(ptr));
    }

    for (uint32_t i = 0; i < nThreads; i++)
    {
        vecThreads.push_back(std::thread(EventLoopThreadPool::ThreadWorkFun, this, i));

    }
    //vecNums = std::vector<int64_t >(nThreads, 0);

}

void EventLoopThreadPool::stop()
{
    bStop = true;
    //
	/*for (int i = 0; i < vecThreads.size(); i++)
	{
		condEvent.notify_all();
	}*/
    for (int i = 0; i < vecThreads.size(); i++)
    {
        if (vecThreads[i].joinable())
            vecThreads[i].join();
    }

    vecThreads.clear();
    vecLoops.clear();

}

IOEventLoop* EventLoopThreadPool::getNextLoop()
{

  std::lock_guard<mutex> guard(loopMutex);
  IOEventLoop* loop = nullptr;

  if (!vecLoops.empty())
  {
    // round-robin
     // printf("%zu \n", nextLoop);
    loop = vecLoops[nextLoop].get();
    ++ nextLoop;
    if (static_cast<size_t>(nextLoop) >= vecLoops.size())
    {
      nextLoop = 0;
    }
  }
  return loop;
}

IOEventLoop* EventLoopThreadPool::getLoopForHash(size_t hashCode)
{
     std::lock_guard<mutex> guard(loopMutex);
  IOEventLoop* loop = nullptr;

  if (!vecLoops.empty())
  {
      size_t index = hashCode % vecLoops.size();
    loop = vecLoops[index].get();
  }
  return loop;
}

///////////////////////////////////////////////


// static function, call Dowork inside
void EventLoopThreadPool::ThreadWorkFun(EventLoopThreadPool * lp, size_t  n)
{
    EventLoopThreadPool * pool = (EventLoopThreadPool * )lp;
	pool->setThreadId(n, std::this_thread::get_id());
    while (bStop != true)
    {

        if (bStop)
        {
           // DEBUG_PRINT("thread index = %zu active and exit \n", n);
            break;
        }
        else
        {
            //DEBUG_PRINT("thread index = %zu active \n", n);
        }

        pool->DoWork(n);
    }
}

void EventLoopThreadPool::setThreadId(size_t index, std::thread::id Id)
{
	assert(index < vecLoops.size());
	vecLoops[index]->setThreadId(Id);
}

void EventLoopThreadPool::DoWork(size_t index)
{
	assert(index < vecLoops.size());

    IOEventLoop *eventLoop = this->vecLoops[index].get();
    eventLoop->loopOnce();
}

