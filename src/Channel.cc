#include "../include/CommonHeader.h"
#include "../include/Channel.h"
#include "../include/IOEventLoop.h"
#include "../include/IPoller.h"

#include <sstream>


using namespace LibNet;

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI;
const int Channel::kWriteEvent = POLLOUT;
const int Channel::kErrorEvent = POLLERR;

Channel::Channel(IOEventLoop *loop, socket_t fd__)
    : fd_(fd__),revents_(0),loop_(loop),
	events_(0),
	index_(-1),
	logHup_(true),
	tied_(false),
	eventHandling_(false),
	addedToLoop_(false)
{
}

Channel::~Channel()
{
	assert(!eventHandling_);
	assert(!addedToLoop_);
	if (loop_)
	{
		assert(!loop_->hasChannel(this));
	}

	readCallback_ = nullptr;
	writeCallback_ = nullptr;;
	closeCallback_ = nullptr;
	errorCallback_ = nullptr;;
}

void Channel::tie(const std::shared_ptr<void> &obj)
{
	tie_ = obj;
	tied_ = true;
}

void Channel::update()
{
	if (events_ == kNoneEvent)
		addedToLoop_ = false;
	else
		addedToLoop_ = true;
	loop_->updateChannel(this);
}

void Channel::remove()
{
	//  assert(isNoneEvent());
	addedToLoop_ = false;
	events_ = kNoneEvent;
	if (loop_)
		loop_->removeChannel(this);
}

void Channel::handleEvent(Timestamp receiveTime)
{
	std::shared_ptr<void> guard;
	if (tied_)
	{
		guard = tie_.lock();  // let sharedPtr refcount++
		if (guard)
		{
			handleEventWithGuard(receiveTime);
		}
	}
	else
	{
		handleEventWithGuard(receiveTime);
	}
}

//
void Channel::handleEventWithGuard(Timestamp receiveTime)
{
	eventHandling_ = true;
	//LOG_TRACE << reventsToString();
	if ((revents_ & POLLHUP) && !(revents_ & POLLIN))
	{
		if (logHup_)
		{
			DEBUG_PRINT("fd = %s \n", Channel::eventsToString().c_str());
		}
		if (closeCallback_)
			closeCallback_();
	}

	if (revents_ & POLLNVAL)
	{
		DEBUG_PRINT("fd = %s \n", Channel::eventsToString().c_str());
	}

	if (revents_ & (POLLERR | POLLNVAL))
	{
		if (errorCallback_)
			errorCallback_();
	}
	// EPOLLRDHUP
	if (revents_ & (POLLIN | POLLPRI | POLLRDHUP))
	{
		if (readCallback_)
			readCallback_(receiveTime);
	}
	if (revents_ & POLLOUT)
	{
		if (writeCallback_)
			writeCallback_();
	}
	eventHandling_ = false;
}

string Channel::reventsToString() const
{
	return eventsToString(fd_, revents_);
}

string Channel::eventsToString() const
{
	return eventsToString(fd_, revents_);
}

string Channel::eventsToString(socket_t  fd, int ev)
{
	std::ostringstream oss;
	oss << "----->";
	oss << "fd=" << fd << ": ";
	if (ev & POLLIN)
		oss << "IN ";
	if (ev & POLLPRI)
		oss << "PRI ";
	if (ev & POLLOUT)
		oss << "OUT ";
	if (ev & POLLHUP)    // close both side
		oss << "HUP ";
	if (ev & POLLRDHUP)  // POLLRDHUP, can write to remote
		oss << "RDHUP ";
	if (ev & POLLERR)
		oss << "ERR ";
	if (ev & POLLNVAL)
		oss << "NVAL ";

	oss << "\n";

	return oss.str();
}
