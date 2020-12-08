#include <iostream>



#include "EventLoop.h"
#include "Thread.h"
#include <stdio.h>
#include <unistd.h>
using namespace muduo;

#include <thread>
EventLoop* g_loop;

void threadFunc()
{
    g_loop->loop();
}

int main()
{
    EventLoop loop;

}
