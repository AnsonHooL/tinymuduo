//
// Created by lenovo on 2020/12/10.
//

#include "EventLoopThread.h"
#include "EventLoop.h"

#include <functional>

EventLoopThread::EventLoopThread()
        : loop_(NULL),
          exiting_(false),
          thread_(std::bind(&EventLoopThread::threadFunc, this)),
          mutex_(),
          cond_(mutex_)
{
}


///漂亮的RAII机制，父线程提前退出，子线程也能join阻塞等待子线程完成任务再离开
EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    loop_->quit();
    thread_.join();
}

///条件变量等待EventLoopThread创建完毕
///返回EventLoop工作线程，one loop per thread，此线程负责Tcpconnection
EventLoop* EventLoopThread::startLoop()
{
    assert(!thread_.started());
    thread_.start();

    {
        MutexLockGuard lock(mutex_);
        while (loop_ == NULL)
        {
            cond_.wait();
        }
    }

    return loop_;
}


///注意EventLoop在栈上初始化，因此EventLoopThreadpool删除的时候不需要delete
///条件变量唤醒start完成初始化
///该工作线程就一直在事件循环loop中，直到线程死亡
void EventLoopThread::threadFunc()
{
    EventLoop loop;
    {
        MutexLockGuard lock(mutex_);
        loop_ = &loop;
        cond_.notify();
    }

    loop.loop();
}