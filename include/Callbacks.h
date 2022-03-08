#ifndef LibNet_NET_CALLBACKS_H
#define LibNet_NET_CALLBACKS_H

#include "./CommonHeader.h"

namespace LibNet
{

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

// should really belong to base/Types.h, but <memory> is not included there.

template<typename T>
inline T* get_pointer(const std::shared_ptr<T>& ptr)
{
  return ptr.get();
}

template<typename T>
inline T* get_pointer(const std::unique_ptr<T>& ptr)
{
  return ptr.get();
}

// Adapted from google-protobuf stubs/common.h
// see License in LibNet/base/Types.h
template<typename To, typename From>
inline ::std::shared_ptr<To> down_pointer_cast(const ::std::shared_ptr<From>& f)
{
  if (f)
    return std::dynamic_pointer_cast<To>(f);

  return nullptr;
}

// All client visible callbacks go here.

//class Buffer;
//class TcpConnection;
//typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
//
//typedef std::function<void(const TcpConnectionPtr&, int stat)> CommonCallback;
//typedef std::function<void (const TcpConnectionPtr&)>  ConnectionEventCallback;
//typedef std::function<void (const TcpConnectionPtr&, size_t)> HighWaterMarkCallback;
//
//// the data has been read to (buf, len)
//typedef std::function<void (const TcpConnectionPtr&, Buffer*, Timestamp)> MessageCallback;

//void defaultConnectionCallback(const TcpConnectionPtr& conn);
//void defaultMessageCallback(const TcpConnectionPtr& conn, Buffer* buffer, Timestamp receiveTime);


}  // namespace LibNet

#endif  // LibNet_NET_CALLBACKS_H
