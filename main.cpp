#include "Channel.h"
#include "EventLoop.h"

#include <stdio.h>
#include <sys/timerfd.h>
#include <unistd.h>

#include "EventLoop.h"

#include <boost/bind.hpp>
#include "Thread.h"
#include <stdio.h>


#include <stdio.h>

EventLoop* g_loop;

void print(){ printf("hello\n");}
void threadFunc()
{
    g_loop->runAfter(1.0,print);
    sleep(1);
    g_loop->quit();
}
int main()
{
    EventLoop loop;
    g_loop = &loop;

    muduo::Thread t(threadFunc);
    t.start();
    loop.loop();
}
