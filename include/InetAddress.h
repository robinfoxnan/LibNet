// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/LibNet/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef LibNet_NET_INETADDRESS_H
#define LibNet_NET_INETADDRESS_H

#include "./CommonHeader.h"

namespace LibNet
{

  namespace socketsApi
  {
    const struct sockaddr *sockaddr_cast(const struct sockaddr_in6 *addr);
  }

  class InetAddress
  {
  public:
    explicit InetAddress(uint16_t port = 0, bool loopbackOnly = false, bool ipv6 = false);

    InetAddress(const string &ip, uint16_t port, bool ipv6 = false);

    explicit InetAddress(const struct sockaddr_in &addr) : addr_(addr)
    {
    }

    explicit InetAddress(const struct sockaddr_in6 &addr) : addr6_(addr)
    {
    }

    sa_family_t family() const { return addr_.sin_family; }
    string toIp() const;
    string toIpPort() const;
    uint16_t port() const;

    // default copy/assignment are Okay

    const struct sockaddr *getSockAddr() const { return socketsApi::sockaddr_cast(&addr6_); }
    void setSockAddrInet6(const struct sockaddr_in6 &addr6) { addr6_ = addr6; }

    uint32_t ipv4NetEndian() const;
    uint16_t portNetEndian() const { return addr_.sin_port; }

    static bool resolve(const string &hostname, InetAddress *result);
    // static std::vector<InetAddress> resolveAll(const char* hostname, uint16_t port = 0);

    // set IPv6 ScopeID
    void setScopeId(uint32_t scope_id);

  private:
    union
    {
      struct sockaddr_in addr_;
      struct sockaddr_in6 addr6_;
    };
  };

} // namespace LibNet

#endif // LibNet_NET_INETADDRESS_H
