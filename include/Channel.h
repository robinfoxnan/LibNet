#ifndef LibNet_NET_CHANNEL_H
#define LibNet_NET_CHANNEL_H

#include "CommonHeader.h"

#include <functional>
#include <memory>

namespace LibNet
{

	class IOEventLoop;

	typedef std::function<void()> EventCallback;
	typedef std::function<void(Timestamp)> ReadEventCallback;

	class Channel
	{
	public:
		static const int kNoneEvent;
		static const int kReadEvent;
		static const int kWriteEvent;
		static const int kErrorEvent;

	public:

		Channel(IOEventLoop *loop, socket_t  fd);
		virtual  ~Channel();

		// this function is called by event poller:
		virtual void handleEvent(Timestamp receiveTime);

		// and send event to 4 kinds of events
		void setReadCallback(ReadEventCallback cb)
		{
			readCallback_ = std::move(cb);
		}
		void setWriteCallback(EventCallback cb)
		{
			writeCallback_ = std::move(cb);
		}
		void setCloseCallback(EventCallback cb)
		{
			closeCallback_ = std::move(cb);
		}
		void setErrorCallback(EventCallback cb)
		{
			errorCallback_ = std::move(cb);
		}

		/// Tie this channel to the owner object managed by shared_ptr,
		/// prevent the owner object being destroyed in handleEvent.
		void tie(const std::shared_ptr<void> &);

		inline socket_t fd() const { return fd_; }
		int events() const { return events_; }
		void set_revents(int revt) { revents_ = revt; } // used by pollers
		// int revents() const { return revents_; }
		bool isNoneEvent() const { return events_ == kNoneEvent; }

		virtual void enableAll()
		{
			events_ |= kReadEvent;
			events_ |= kWriteEvent;
			update();
		}
		void enableReading()
		{
			events_ |= kReadEvent;
			update();
		}
		void disableReading()
		{
			events_ &= ~kReadEvent;
			update();
		}
		void enableWriting()
		{
			events_ |= kWriteEvent;
			update();
		}
		void disableWriting()
		{
			events_ &= ~kWriteEvent;
			update();
		}
		void disableAll()
		{
			events_ = kNoneEvent;
			update();
		}
		bool isWriting() const { return events_ & kWriteEvent; }
		bool isReading() const { return events_ & kReadEvent; }

		// for Poller
		int index() { return index_; }
		void set_index(int idx) { index_ = idx; }

		// for debug
		string reventsToString() const;
		string eventsToString() const;

		void doNotLogHup() { logHup_ = false; }

		IOEventLoop *ownerLoop() { return loop_; }
		void remove();

		
	protected:
		socket_t fd_;
		int revents_; // it's the received event types of epoll or poll

		ReadEventCallback readCallback_;
		EventCallback writeCallback_;
		EventCallback closeCallback_;
		EventCallback errorCallback_;
	private:
		static string eventsToString(socket_t  fd, int ev);

		void update();
		void handleEventWithGuard(Timestamp receiveTime);
		IOEventLoop *loop_;
		int events_;
		int index_;   // used by Poller.
		bool logHup_;

		std::weak_ptr<void> tie_;
		bool tied_;
		bool eventHandling_;
		bool addedToLoop_;
		
	};

} // namespace LibNet

#endif // LibNet_NET_CHANNEL_H
