各个函数的返回值

## 1 connnect

13  EACCES, EPERM 用户试图在套接字广播标志没有设置的情况下连接广播地址或由于防火墙策略导致连接失败。
98 EADDRINUSE本地地址处于使用状态。
97 EAFNOSUPPORT参数serv_add中的地址非合法地址。
11 EAGAIN没有足够空闲的本地端口。
114 EALREADY套接字为非阻塞套接字，并且原来的连接请求还未完成。
9EBADF 非法的文件描述符。
111 ECONNREFUSED远程地址并没有处于监听状态。
14 EFAULT  指向套接字结构体的地址非法。
115 EINPROGRESS套接字为非阻塞套接字，且连接请求没有立即完成。
4 EINTR  系统调用的执行由于捕获中断而中止。
106 EISCONN  已经连接到该套接字。
101 ENETUNREACH网络不可到达。
88 ENOTSOCK  文件描述符不与套接字相关。
110 ETIMEDOUT  连接超时
