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
#include "Mutex.h"

#include <vector>
#include <memory>
using namespace muduo;

class Channel;
class Poller;
class EPoller;

class TimerQueue;

class EventLoop : public noncopyable
{
public:
    typedef std::function<void()> Functor;

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


    /// Runs callback immediately in the loop thread.
    /// It wakes up the loop, and run the cb.
    /// If in the same loop thread, cb is run within the function.
    /// Safe to call from other threads.
    void runInLoop(const Functor& cb);
    /// Queues callback in the loop thread.
    /// Runs after finish pooling.
    /// Safe to call from other threads.
    void queueInLoop(const Functor& cb);

    /// timers

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

     void cancel(TimerId timerId);

    /// internal use only
    void wakeup();
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);

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
    void handleRead();  // waked up
    void doPendingFunctors();

    typedef std::vector<Channel*> ChannelList;

    bool looping_; /* atomic */
    bool quit_; /* atomic */
    bool callingPendingFunctors_; /* atomic */

    const pid_t threadId_;
    Timestamp pollReturnTime_;
//    std::unique_ptr<Poller> poller_;
    std::unique_ptr<EPoller> poller_;
    std::unique_ptr<TimerQueue> timerQueue_;
    ChannelList activeChannels_;

    ///用来run in loop，其他线程安全的注册函数，进行异步调用（因此需要加锁）
    int wakeupFd_;
    std::unique_ptr<Channel> wakeupChannel_;
    MutexLock mutex_;
    std::vector<Functor> pendingFunctors_; // @GuardedBy mutex_
};








#endif //TINYMUDUO_EVENTLOOP_H
