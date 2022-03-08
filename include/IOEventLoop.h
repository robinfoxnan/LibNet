#ifndef LibNet_NET_EVENTLOOP_H
#define LibNet_NET_EVENTLOOP_H

#include "./CommonHeader.h"
//#include "./Callbacks.h"
#include "TimerHeap.h"

namespace LibNet
{

  class Channel;
  class IPoller;
  typedef std::function<void()> Functor;
    typedef std::vector<Channel *> ChannelList;

  class IOEventLoop
  {
  public:

    IOEventLoop();
    ~IOEventLoop();

	int64_t  addTimer(int64_t delayMs, int interval, TimerCallback cb);
	void     delTimer(int64_t id);
    // 1) call poller function
    // 2) exec functors
    void loopOnce();



    //Timestamp pollReturnTime() const { return pollReturnTime_; }


    void runInLoop(Channel *ch, Functor cb);
    void queueInLoop(Channel *ch, Functor cb);

    size_t queueSize();


    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);
    bool hasChannel(Channel *channel);

    // pid_t threadId() const { return threadId_; }
	void setThreadId(std::thread::id Id)
	{
		this->threadId = Id;
	}
    void assertInLoopThread()
    {
		bool ret = (threadId == std::this_thread::get_id());
      if (! ret)
      {
        abortNotInLoopThread();
      }
    }

    bool isInLoopThread() const
    {
		bool ret = (threadId == std::this_thread::get_id());
        return ret;
    }


    bool isEventHandling() const { return bEventHandling; }

//    void setContext(const boost::any &context)
//    {
//      context_ = context;
//    }

//    IContext * getContext()
//    {
//      return &context_;
//    }

    static IOEventLoop *getIOEventLoopOfCurrentThread();

  private:
    void abortNotInLoopThread();
    void handleRead(); // waked up
    void doPendingFunctors();

    void printActiveChannels() const; // DEBUG

  private:
    // scratch variables
    ChannelList activeChannels;
    Channel *currentActiveChannel;

    std::atomic<bool> bEventHandling;          /* atomic */
    std::atomic<bool> bCallingPendingFunctors; /* atomic */

    std::thread::id threadId;
    Timestamp pollReturnTime;

    std::unique_ptr<IPoller> poller;

    // store all funtors to be executed
	using Job = std::pair<Channel *, Functor>;
    std::mutex mutexQue;
    std::deque<Job> functorQue;

	// with lock inside, thread safe
	std::unique_ptr<TimerHeap> timerHeap;
  };

} // namespace LibNet

#endif // LibNet_NET_EVENTLOOP_H
