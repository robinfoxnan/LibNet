#ifndef LibNet_NET_TCPCONNECTION_IOCP_H
#define LibNet_NET_TCPCONNECTION_IOCP_H

#include "../CommonHeader.h"
#include "../TcpConnection.h"

namespace LibNet
{


	class TcpConnectionIocp : public TcpConnection//, public std::enable_shared_from_this<TcpConnectionIocp>
	{
	public:
		TcpConnectionIocp(const string & ip, int port);
		virtual ~TcpConnectionIocp() override;


		virtual bool send(const char* data, int len, void *pVoid) override;
		virtual void initSocket() override;
		virtual void startReadWrite();

		virtual void handleWriteEvent() override;
		virtual void handleConnectEvent()  override;
		virtual void handleReadEvent(Timestamp receiveTime)  override;
		virtual void handleCloseEvent()  override;

	private:
		void preRead();

	};

	using TcpConnectionIocpPtr = std::shared_ptr<TcpConnectionIocp>;


}  // namespace LibNet

#endif  // LibNet_NET_TCPCONNECTION__IOCP_H
