# 设计思路

## 参考：

1、muduo ： 陈硕写的linux异步库，缺点是使用了boost，而且不支持其他平台；
2、handy :  
3、zsummerX：  log4z的作者发布的源码；
4、libuv：nodejs底层库；
5、

## 集成的第三方源码：

1、log4z :
2、log4cpp: 
3、uvcpp的日志部分：
4、http_parser: 

## 一、总体设计

**设计目的**：用c++11写一个轻量级跨平台（windows，linux, mac )的高性能网络库，用于高速的rpc调用；

**设计原则**：尽可能少的使用第三方二进制库，保证整体代码可以以源码形式嵌入到目标工程中；优先考虑linux的系统特性，兼容windows特性；

**结构如下图所示**：

![](image\total.png)





整个库的基础结构是构建一个线程池的基础上，所有的上层的socket业务都工作在某个线程中，也就是某个socket在诞生之初，就分配给一个线程，（分配算法可以通过线程池分配函数控制）这样设计的好处是：

1）每个线程一个loop循环，每个循环使用一个select，或者epoll，避免模型本身的一些问题；比如惊群以及select数组限制；

2)  在loop内的线程的socket都在线程上工作，不需要考虑线程安全问题；（个别用户异步命令需要投递到线程上处理，类似libuv）;

3）单个循环上socket的操作不进行线程切换，





## 一、Channel

### 1.1 定义

Channel 描述一个socket或者异步文件描述符；

在linux中文件、管道、套接字都使用int 类型；
而windows中使用SOCKET来描述；
这里统一重定义为socket_t类型；

Channel是包含3个成员：

```c++
socket_t fd_;   // 文件描述符
int      event; // 关心的事件掩码
int      revent;// 当前已经发生的事件
```

Socket继承了此类，仅仅是添加了一些设置Socket相关的操作；而读写等一些方法并没有放进去；

### 1.2 生命周期与释放

Channel归属Connection来管理，或者Server来管理，本身是shared_ptr，但是在IPoll中管理时，以及IOEventPoll分发当前事件时，却使用了原始的指针类型Channel *，这是因为考虑到shared_ptr会消耗更多的资源；比如一个服务器，需要上万个连接，消耗还是挺大的；

但是需要保证Channel的生命周期结束时，必须已经从IOEventLoop中移除了，否则，后续使用指针就会崩溃；

隐含条件是：IOEventLoop的生命周期长，持续整个涉及IO的全过程，不能先退出！！！

各个使用IOEventLoop的地方也都是使用裸指针，因为隐含的生命周期保证了一定是可用的！！！



redis中没有考虑这些东西，因为整个就是一个线程在工作，所以没有啥线程安全需要考虑的。

## 二、SocketsAPi
不同平台的socket使用方法不近相同，这里对读写进行了重新封装；




## 三、IPoller

IPoller是用来检查socket中是否存在事件需要处理的类；几乎所有的异步IO库都是使用一个线程池使用一个检查类来干这个；比如Redis中每种事件驱动模型定义了单独的驱动类

```c++
ae_select.c

ae_epoll.c
```

一般是一个种类实现一个单独的类；而该类实现的功能非常的简单

- updateChannel ：添加描述符，设置关心的事件；
- removeChannel ：移除描述符，
-  poll ：查询描述符中当前是否有事件；

IPoll定义了一个内部成员

```c++
std::map<socket_t, Channel *> channels_;
```

之所以需要此成员，是因为IPooler需要在获取了socket上的事件后，需要与socket对应的上下文对应起来；在IOEventloop中分发事件，调用回调函数；

poll、epoll、select等模型都么有这样的机制；

而IOCP模型则太需要，因为IOCP直接绑定socket与完成端口，并且关联了上下文，并且是线程安全的；

##  





四、windows与linux系统的select区别

fd_set在2种操作系统下的实现方式差别比较大：

linux下面定义，对宏翻译后大概是这样的：

```c++
typedef struct
  {
    // 支持1024个文件描述符，而用bit作为掩码使用，每个long是64位，所以除以64
    long int fds_bits[1024 / (8 * 8))];  
  } fd_set;
```

所以使用掩码方式可以直接按比特位进行设置，和删除；

```c
int select(int nfds, fd_set *readfds, fd_set *writefds,
                  fd_set *exceptfds, struct timeval *timeout);
```

man手册中说明，nfds并不是我们设置了多少个socket，而是最大的fd + 1;

这是因为

1）socket本身就是int从小到大的使用，当设置了最大的，最大值可以保证小的都被覆盖；

2）按照掩码方式使用，设置了大值，之前的都是需要监测的；



windows不同：

```c++
#define FD_SETSIZE      64
#endif /* FD_SETSIZE */

typedef struct fd_set {
        u_int fd_count;               /* how many are SET? */
        SOCKET  fd_array[FD_SETSIZE];   /* an array of SOCKETs */
} fd_set;
```

这里使用了一个很土的数组，加上一个使用的个数；

设置时候是

1）监测是否设置了；

2）如果找不到，则追加；

删除的时候是：

1）找到当前位置；

2）逐个向前挪动；

效率比linux的方式低了很多啊，

所以在使用select时候nfds的设置其实就是实际添加了多少个socket；



**总结：为啥微软不采用掩码方式实现？是微软太笨么？**

**不是，是因为windows下socket的值并不是从小到大的一个递增的小整数，值可能会非常的大，所以不能使用小数组实现掩码。**