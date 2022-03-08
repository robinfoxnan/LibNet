#pragma once
#include <windows.h>

#include "../IPoller.h"
#include "../Channel.h"
#include "IocpContex.h"

namespace LibNet
{

	class IocpPoller : public IPoller
	{
	public:
		IocpPoller(IOEventLoop *loop);
		virtual ~IocpPoller() override;

		virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;
		virtual void updateChannel(Channel *channel) override;
		virtual void removeChannel(Channel *channel) override;

	private:
		HANDLE iocpHandle;
		void addActiveChannel(IocpContext * context, ChannelList* activeChannels);

	};
}

