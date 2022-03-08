#include <QCoreApplication>
#include "./include/EventLoopThreadPool.h"
#include "./include/IOEventLoop.h"

using namespace  muduo;
int main(int argc, char *argv[])
{

    muduo::EventLoopThreadPool::instance().Init(3);
    muduo::EventLoopThreadPool::instance().start();

    for (int i =0 ; i<100; i++)
    {
        IOEventLoop* loop = muduo::EventLoopThreadPool::instance().getNextLoop();
        if (loop)
        {
            loop->runInLoop([i](){

                printf("functio run i=%d\n", i);
            });
        }
    }



     muduo::EventLoopThreadPool::instance().stop();

    return 0;
}
