//
// Created by lenovo on 2020/12/8.
//

#ifndef TINYMUDUO_EVENTLOOP_H
#define TINYMUDUO_EVENTLOOP_H

#include "noncopyable.h"
#include "Thread.h"

using namespace muduo;

class EventLoop : public noncopyable
{
public:
    EventLoop();
    ~EventLoop();

    void loop();

    void assertInLoopThread()
    {
        if (!isInLoopThread())
        {
            abortNotInLoopThread();
        }
    }
    bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }

private:
    void abortNotInLoopThread();
    bool looping_; /* atomic */
    const pid_t threadId_;



};








#endif //TINYMUDUO_EVENTLOOP_H
