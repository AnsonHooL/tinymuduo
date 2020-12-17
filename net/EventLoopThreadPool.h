//
// Created by lenovo on 2020/12/17.
//

#ifndef TINYMUDUO_EVENTLOOPTHREADPOOL_H
#define TINYMUDUO_EVENTLOOPTHREADPOOL_H


#include "Condition.h"
#include "Mutex.h"
#include "Thread.h"
#include "noncopyable.h"


#include <vector>
#include <functional>
#include <memory>

class EventLoop;
namespace muduo{
    class EventLoopThread;
}


class EventLoopThreadPool : noncopyable
{
public:
    EventLoopThreadPool(EventLoop* baseLoop);
    ~EventLoopThreadPool();
    ///先设置ThreadNum，再启动
    void setThreadNum(int numThreads) { numThreads_ = numThreads; }
    void start();
    EventLoop* getNextLoop();

private:
    EventLoop* baseLoop_;
    bool started_;
    int numThreads_;
    int next_;  // always in loop thread
    std::vector<std::unique_ptr<muduo::EventLoopThread>> threads_;
    std::vector<EventLoop*> loops_;
};




















#endif //TINYMUDUO_EVENTLOOPTHREADPOOL_H
