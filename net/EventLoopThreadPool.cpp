//
// Created by lenovo on 2020/12/17.
//

#include "EventLoopThreadPool.h"

#include "EventLoop.h"
#include "EventLoopThread.h"

#include <functional>

using namespace muduo;

/// 初始化默认是单Reactor模式，连接 + I/O
/// 传入一个baseLoop负责监听连接，其他线程都是I/O连接
EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop)
  :baseLoop_(baseLoop),
   started_(false),
   numThreads_(0),
   next_(0)
{

}

///baseLoop是栈对象不需要析构
///其他的Eventloopthread是uniquePtr管理
///完美使用智能指针，不需要显示析构~
EventLoopThreadPool::~EventLoopThreadPool()
{
    // Don't delete loop, it's stack variable
}


///构造n个IO线程，用uniqueptr管理
void EventLoopThreadPool::start()
{
    assert(!started_);
    baseLoop_->assertInLoopThread();

    started_ = true;

    for (int i = 0; i < numThreads_; ++i)
    {
        EventLoopThread* t = new EventLoopThread;
        threads_.push_back(std::unique_ptr<EventLoopThread>(t));
        loops_.push_back(t->startLoop());
    }
}

///轮询获取下个IO线程，eventloop，管理Tcpconn
EventLoop* EventLoopThreadPool::getNextLoop()
{
    baseLoop_->assertInLoopThread();
    EventLoop* loop = baseLoop_;

    if (!loops_.empty())
    {
        // round-robin
        loop = loops_[next_];
        ++next_;
        if (static_cast<size_t>(next_) >= loops_.size())
        {
            next_ = 0;
        }
    }
    return loop;
}