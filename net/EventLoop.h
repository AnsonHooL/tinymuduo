//
// Created by lenovo on 2020/12/8.
//

#ifndef TINYMUDUO_EVENTLOOP_H
#define TINYMUDUO_EVENTLOOP_H

#include "Timestamp.h"
#include "Callbacks.h"
#include "TimerId.h"
#include "noncopyable.h"
#include "Thread.h"
#include <vector>
#include <memory>
using namespace muduo;

class Channel;
class Poller;
class TimerQueue;

class EventLoop : public noncopyable
{
public:
    EventLoop();

    // force out-line dtor, for scoped_ptr members.
    ~EventLoop();
    ///
    /// Loops forever.
    ///
    /// Must be called in the same thread as creation of the object.
    ///
    void loop();

    void quit();

    ///
    /// Time when poll returns, usually means data arrivial.
    ///
    Timestamp pollReturnTime() const { return pollReturnTime_; }

    // timers

    ///
    /// Runs callback at 'time'.
    ///
    TimerId runAt(const Timestamp& time, const TimerCallback& cb);
    ///
    /// Runs callback after @c delay seconds.
    ///
    TimerId runAfter(double delay, const TimerCallback& cb);
    ///
    /// Runs callback every @c interval seconds.
    ///
    TimerId runEvery(double interval, const TimerCallback& cb);

    // void cancel(TimerId timerId);

    // internal use only
    void updateChannel(Channel* channel);
    // void removeChannel(Channel* channel);

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

    typedef std::vector<Channel*> ChannelList;

    bool looping_; /* atomic */
    bool quit_; /* atomic */
    const pid_t threadId_;
    Timestamp pollReturnTime_;
    std::unique_ptr<Poller> poller_;
    std::unique_ptr<TimerQueue> timerQueue_;
    ChannelList activeChannels_;
};








#endif //TINYMUDUO_EVENTLOOP_H
