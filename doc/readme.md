
# 说明

现象
qt调试终端程序，当有创建线程的时候，收到信号SIGSTOP 而退出，无法调试程序。

解决
解决方式是，设置GDB不处理SIGSTOP ，
在QtCreator中进入GDB命令设置窗口：
Tools -> Options -> Debugger -> GDB -> Additional Startup Command
在 Additional Startup Command 中添加如下内容：

'''handle SIGSTOP nostop pass'''
## 当前的设计
### 1、Channel与Socket
通道管理一个文件描述符，而描述符上开启能否收发的的开关命令；
则理论上，模型可以实现对文件、管道、socket等各种异步IO的操作的封装；
Socket继承于Channel，封装了除了read和write以外的其他相关业务功能；
之所以要独立处理，是因为读写的业务逻辑相对更加复杂，而且与接口的设计有关，业务缓存的内存使用方式有关；

Channel将自己注册到关联的IOEventLoop中，通过该IOEventLoop来回调自己的事件处理函数；
handleEvent将事件细分，再通过上层业务（client\server)设置的回调函数来通知上层处理；
比如客户端Socket由上层TcpConnection提供回调函数处理四种事件；

### 2、IOEventLoop
这个类并没有线程，也没有loop，而只实现了用于一次循环操作所需要的操作；
loopOnce函数的流程如下：
1）等待所管理的socket描述符，等待是否有事件需要处理；
2）如果有事件需要处理，则针对有事件的socket，逐个调用回调函数处理；
   这里的回调函数中，通过channal的handleEvent函数分发为读、写、关闭、错误四种情况；
3）循环处理所有等待处理的闭包(functor)函数，这些闭包函数在此IOEventLoop所在线程中顺序执行，所以不再需要锁来保证线程安全；这里的闭包的功能可能包括：socket上的各种命令，上层发起连接、上层发起关闭、上层发起发送、对socket收发的检测开启与关闭等；
-
### 3、EventLoopThreadPool
 这里的线程池，内部管理N个线程以及N个IOEventLoop，一 一对应；每个线程使用自己的IOEventLoop中的loopOnce来完成对socket的管理与操作；这里的线程仅仅是一个容器而已；功能都在IOEventLoop封装；

### 4、TcpConnection
这里的接口与内存使用了libuv类似的方式：

1）当有数据可以读取时，提前调用上层的BeforeRead回调函数，用户可以自己管理内存，设置内存地址、长度、以及上下文环境的指针；
读取后，用户自己负责释放内存；

2）如果用户没有设置此回调，则TcpConnnection内部封装了Buffer类，提供了4096字节的内存，读完之后会通知用户处理，该内存由TcpConnnection类管理，用户可以只负责读取；或者从上下文中获取Buffer类，使用swap操作交换内存到自己的Buffer类中，自己释放Buffer;

## 线程安全问题

### 1、epoll是线程安全的么？
https://blog.csdn.net/u011344601/article/details/51997886
https://zhuanlan.zhihu.com/p/30937065

我听说过的线程模型主要是2种：
1）每个IO线程使用一个epoll循环，线程上管理多个socket，socket的读写事件处理逻辑直接在IO线程上处理；
这样设计不太适合使用ET模式，因为此处理模式的常规操作是不停的读写，直到出现需要等待的提示；如果某个socket的读写一直无法中断，则肯定会阻塞其他的socket，造成响应不及时；

2）与模式1）不同的在于，socket出现读写事件后，将事件处理发送到其他的事件处理线程上执行操作；这样的设计模式适合单个socket操作时间更长的应用模式；这样的设计相对适合ET方式，但是也要适当的使用策略规避其他socket的等待时间过长问题；

按照网上讨论的，在epoll_wait时出现调用epoll_ctl是线程安全的，
但是如果此时本地上层业务直接close操作，可能造成预测不到的后果；
所以
1）channel.enableWrite()是线程安全的；可以不在循环中调用；
2）channel.enableRead()是线程安全的；可以不在循环中调用；
3) Socket.close() 还是要异步放到线程中做相对安全；
4) read（）理论上是安全的；但是目前设计中并没有让用户主动的去读；
5) write() 有可能需要将无法写入的数据添加发送队列，如果不加锁，还是需要放到线程中操作；

### 2、select 线程安全

select模型的好处就是多个平台基本都有实现，兼容性比较好；

select函数需要操作fdset结构体，该结构体的操作对应三个宏，

FD_SET就是遍历数组，找到空位置，添加socket;

FD_CLR就是找到socket，数组之后的依次向前挪动；

数组大小通过宏来控制，windows下默认是64，需要重定义宏，保证够用；

在数组中添加或者删除socket，都不是线程安全的，所以相关操作应该都在线程中调用；

大家的实现方式起始都是类似的，没有啥新鲜的东西；可以参考redis的

ae_select.c 源码；

### 3、poll 线程安全

### 4、IOCP 线程安全



备注：

1）   SO_REUSEPORT和SO_REUSEADDR

 https://blog.csdn.net/Yaokai_AssultMaster/article/details/68951150

https://stackoverflow.com/questions/14388706/how-do-so-reuseaddr-and-so-reuseport-differ/14388707#14388707

2）