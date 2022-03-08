#ifndef LibNet_NET_TCPCONNECTION_H
#define LibNet_NET_TCPCONNECTION_H

#include "../include/CommonHeader.h"
#include "../include/StringPiece.h"

#include "../include/Callbacks.h"
#include "../include/Buffer.h"
#include "../include/InetAddress.h"
//#include "../include/Callbacks.h"


//struct tcp_info;

namespace LibNet
{

	class Channel;
	class IOEventLoop;
	class Socket;

	typedef struct write_req {

	public:
		write_req(char * p, size_t sz, void * lp) : data(p), len(sz), begin(0), param(lp)
		{
		}

		write_req(char * p, size_t sz, size_t b, void * lp) : data(p), len(sz), begin(b), param(lp)
		{
		}

		char * data;
		size_t  len;
		size_t begin;
		void * param;

	}WriteReq;


	class TcpConnection;
	using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
	using  ConnectionEventCallback =std::function<void(const TcpConnectionPtr&)>;
	using  BeforeReadCallback = std::function<void(const TcpConnectionPtr &, char * *data, size_t *sz, void **pVoid)>;
	using  AfterReadCallback = std::function<void(const TcpConnectionPtr &, char * data, size_t sz, void *pVoid)>;
	using  AfterWriteCallback = std::function<void(const TcpConnectionPtr &, const char * data, size_t sz, void *pVoid, int status)>;
	using  AfterConnectionCallback = std::function<void(const TcpConnectionPtr &)>;
	using  AfterCloseCallback = std::function<void(const TcpConnectionPtr &)>;

	class TcpConnection : public std::enable_shared_from_this<TcpConnection>
	{
	public:
		TcpConnection(const string & ip, int port);
		virtual ~TcpConnection();

		void setSecurityMode();
		int  getLastErr();
		string  getLastErrInfo();

		bool clearCallBack();
		void setClientCallback(BeforeReadCallback cliAlloc, AfterReadCallback cliRead, AfterWriteCallback cliWrite, AfterCloseCallback cliClose);

		bool connect(int secs, ConnectionEventCallback cb);
		bool connectSyn(int secs);

		bool isConnected() const { return currentStat == kConnected; }
		bool isDisconnected() const { return currentStat == kDisconnected; }
		void setTcpNoDelay(bool on);

		bool close();      // asyn
		bool closeSyn();   // syn	

		const InetAddress& getLocalAddress() const { return localAddr; }
		const InetAddress& getPeerAddress() const { return peerAddr; }

		// return true if success.
		//bool getTcpInfo(struct tcp_info*) const;
		//string getTcpInfoString() const;
		const char* stateToString() const;

		virtual bool send(const char* data, int len, void *pVoid);

		IOEventLoop* getLoop();

	protected:
		virtual void initSocket();
		bool bUseSSL;
		IOEventLoop* localLoop;   // socket (channel )in  eventloop
		std::shared_ptr<Socket> localSocket;

		void startWrite();
		void stopWrite();
		void connecting();
		void startRead();
		void stopRead();
		virtual void startReadWrite();   // enable chanle in poller

	public:
		
		//bool shutdown(ConnectionEventCallback cb);

		// events, called by socket run in loop
		void onClose();
		void onError();
		void onWrite();
		void onRead(Timestamp  t);

		// events really do work
		virtual void handleWriteEvent();
		virtual void handleConnectEvent();
		virtual void handleReadEvent(Timestamp receiveTime);
		virtual void handleCloseEvent();

	protected:
		AfterConnectionCallback clientConnCallback;   // upper client
		BeforeReadCallback clientAllocCallback;
		AfterReadCallback  clientReadCallback;
		AfterWriteCallback clientWriteCallback;
		AfterCloseCallback clientCloseCallback;

		AfterCloseCallback closeCallbackSynWait;     // only used by SynClose()

		// void send(const StringPiece& message);
		// void send(Buffer* message);		

		enum States { kDisconnected, kConnecting, kConnected, kDisconnecting };
		void setState(States s) { currentStat = s; }
		States currentStat;
		void realSend(const char* data, size_t len, void *pVoid);


	protected:
		string remoteIP;       // save remote server info
		int    remotePort;
		

		InetAddress localAddr;
		InetAddress peerAddr;

		
		int lastError;            // last err
		string errInfo;           // last err info string

		Buffer readBuffer;
		// access it in loop thread, handleWrite pop, realSend push
		std::deque<WriteReq> outputQue;
	};

	//typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;


}  // namespace LibNet

#endif  // LibNet_NET_TCPCONNECTION_H
