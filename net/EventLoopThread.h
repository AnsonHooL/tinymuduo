//
// Created by lenovo on 2020/12/10.
//

#ifndef TINYMUDUO_EVENTLOOPTHREAD_H
#define TINYMUDUO_EVENTLOOPTHREAD_H

#include "Condition.h"
#include "Mutex.h"
#include "Thread.h"

#include "noncopyable.h"

class EventLoop;

namespace muduo
{

class EventLoopThread : noncopyable
{
public:
    EventLoopThread();
    ~EventLoopThread();
    EventLoop* startLoop();

private:
    void threadFunc();

    EventLoop* loop_;
    bool exiting_;
    Thread thread_;
    MutexLock mutex_;
    Condition cond_;
};

}

























#endif //TINYMUDUO_EVENTLOOPTHREAD_H
