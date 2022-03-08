# HttpClient类使用与设计说明

## 一、目前支持3种模式：

1）同步访问；

```c++
void testSynGet()
{
	LibNet::EventLoopThreadPool::instance().Init(1).start();

	string ip = "220.181.38.251";
	ip = "10.128.6.236";
	int port = 80;
	HttpClient httpClient(ip, port);
	httpClient.setCmdTimeout(2);

	int N = 300;
	for (int i = 0; i < N; i++)
	{
		char path[260];
		snprintf(path, 260, "/test.php?id=%d", i + 1);
		int64_t id = httpClient.Get(path);
		if (id > 0)
		{
			printf("id = %lld, code = %d, body = {%s} \n",
				id, 
				httpClient.getResponse()->code,
				httpClient.getResponse()->body.c_str());
		}
		else
		{
			int err = httpClient.getLastErr();
			if (err == Error::Unreachable)
			{
				N = i;
				printf("id = %d cant'connect host= %s:%d, exit here \n",
					i+1, ip.c_str(), port);
				break;
			}
			else if (err == Error::Timeout)
			{
				printf("id = %d timeout\n", i + 1);
			}
			else
			{
				printf("id = %d err = %d\n", i + 1, err);
			}
		}
	}

	httpClient.closeConnection(); 

	LibNet::EventLoopThreadPool::instance().stop();
}
```

2）异步单次访问：

```c++
tets
```

3）流水线访问

```c++

```



## 二、工作模式的区别与httpserver造成的错误

HTTP请求最大的问题在于处理发生异常时的超时或者无响应，以及长链接的定时关闭机制：

HttpClient类，各种模式需要解决的问题以及测试：

1、模拟延时

在apache上添加脚本，请求方式：http://localhost:80/test.php?id=1

某个id的请求，执行延时测试

```php
<?php

// localhost:80/test.php?id=1

$id = '';
if (isset($_GET['id']))
{
	$id = $_GET['id'];
	$num = intval($id);
	
	if ($num % 22 === 0)   // 此次会造成超时
		sleep(5);
}
echo "Hello world, id =  $id ";
```

2、模拟长链接关闭

比如使用小皮面板可以设置单个长连接的响应次数：
<div  align="center">
<img src="image\keep_alive_times.png" width = "600" height = "400" alt="长链接设置" />
</div>


同步访问有3种情况返回：

1）解析结束onEnd，调用responseCallback的lamda激活std::promise返回；

2）std::promise同步等待超时，则返回；

3）连接被重置，也会造成std::promise同步等待超时，以超时的错误返回；



异步和流水线模式，也有3种情况返回：2种模式工作模式很相近；

1）解析结束onEnd，调用responseCallback的lamda，lamda并通知用户单个结束；lamda内置于enablePipeling()函数内；（无锁，用户可以直接发送新的request)

2）连接被重置，设置错误并调用notifyUserError()；notifyUserError函数会无锁通知所有未完成的请求都失败了；

3）定时器回调函数检查请求队列首个请求超时，设置错误并调用notifyUserError()；



此2种模式与同步处理的区别主要在于：

1）responseCallback 的lamda表达式不一样，所以解析response结束后调用的实际业务逻辑不同；

2）同步处理使用currentReq，而不使用请求队列；

3）同步处理不需要使用定时器；



在异步和流水线模式中定时器的使用：

开启时机：统一的入口函数sendRequestInPipeline（），需要检查是否需要添加定时器，首次会添加；

关闭时机：用户调用closeConnection()，或者HttpClient析构时，会关闭定时器，

备注：

1）onEnd返回后，检查是否所有的队列已经空了；但是为了提高效率，不检查关闭定时器；

2）超时和重置连接，Connection类本身并不会重置，所以不需要关闭；

3）定时器与Connection在同一IOEventLoop上，可以保证相关逻辑代码是顺序执行；

4）定时器启动后，生命周期伴随Connection主要是为了防止异步冲突，比如如果频繁的添加关闭定时器，可能出现当类内部正在关闭定时器，而用户正好在发送数据而启动定时器，此时会冲突，





1. 同步访问：

| 问题列表                | 解决方案                                                     |
| ----------------------- | ------------------------------------------------------------ |
| 1）无法连接；           | 连接超时直接返回<br/>默认连接时间5秒                         |
| 2）长链接被服务器关闭； | 通过std::promise等待超时<br/>                                |
| 3）响应超时             | httpClient.setCmdTimeout(2);<br/>通过std::promise机制获得超时，<br/>默认是3秒 |
| 4）同步等待             | 类成员添加一个std::promise                                   |

2. 异步单次访问：

   基本逻辑与流水线的逻辑差不多，唯一就是用户接口有少许区别；

   

3. 流水线访问；

   通过回调函数得知发送结果：成功（仅仅是返回了，具体要查看code），超时，远端关闭；

| 问题列表                | 解决方案                                                     |
| ----------------------- | ------------------------------------------------------------ |
| 1）无法连接；           | 连接超时直接返回<br/>默认连接时间5秒                         |
| 2）长链接被服务器关闭； | Connection会调用HttpClient::onConnectionClose()<br/>再通过pipeResCallback通知所有当前已经发送的request都超时失败了；<br/>用户层通过Response的错误号得知；然后重新发送；用户不能通过回调函数发送，因为HttpClient当前发送队列被加锁了；<br/>onClose()，检查request队列中是否有数据，需要逐个通知每个request出错了； |
| 3) 响应超时             | 核心的发送函数为：HttpClient::sendRequestInPipeline()<br/>当isPipeline时，request会添加到队列中，异步也会；同步请求不会；<br/>同时设置一个计时器； |
|                         |                                                              |
|                         |                                                              |
|                         |                                                              |

备注：

httpClient的异步发送函数不是线程安全的，所以不能异步的发送数据：

1）流水线模式，需要设置一个外部队列持续发送，出错的（超时，被断开）重新添加到队列；

2）异步模式，发送第一个后，后续可以在异步回调中重发或者发送一个新的请求；



## 三、性能测试

长时间的RPC测试，保证长连接而且能不发生服务器主动断开连接的情况，建议使用golang编写服务端测试：

代码如下：

```go
package main
import (
    "fmt"
    "net/http"
)
 
func IndexHandlers(w http.ResponseWriter, r *http.Request){

	// fmt.Println("============================")
	 id := r.Header["Pipeline-Id"][0]
	// if len(r.Header) > 0 {
    //   for k,v := range r.Header {
	// 	if (k == "Pipeline-Id"){
	// 	id = v[0]
	// 	}
    //      fmt.Printf("%s=%s\n", k, v[0])
    //   }
	// }
	// fmt.Println("");
    fmt.Fprintln(w, "hello, world =========> " + id)
}
 
func main (){
    http.HandleFunc("/", IndexHandlers)
	http.HandleFunc("/hi", IndexHandlers)
    err := http.ListenAndServe("10.128.6.15:80", nil)
    if err != nil {
        fmt.Printf("listen error:[%v]", err.Error())
    }
}
```

