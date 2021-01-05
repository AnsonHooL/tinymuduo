//
// Created by lenovo on 2020/12/8.
//
#include "EventLoop.h"

#include "Channel.h"
#include "Poller.h"
#include "EPoller.h"
#include "TimerQueue.h"

#include "Logging.h"
#include "noncopyable.h"

#include <assert.h>
#include <sys/eventfd.h>
#include <signal.h>
using namespace muduo;

///TLS thread local storage 每个线程有一份自己的Eventloop指针
///注意tls存储不是什么都能存储，POD，plain old data
__thread EventLoop* t_loopInThisThread = 0;
const int kPollTimeMs = 10000;

///创建事件fd
static int createEventfd()
{
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
    {
        LOG_SYSERR << "Failed in eventfd";
        abort();
    }
    return evtfd;

}

///屏蔽掉SISGPIPE信号
///服务器可能因为忙，没有及时处理对方断开连接事件，在断联后继续发送数据，就会被终止进程
class IgnoreSigPipe
{
public:
    IgnoreSigPipe()
    {
        ::signal(SIGPIPE, SIG_IGN);
    }
};

IgnoreSigPipe initObj;

///监听wakeup_fd，说明有异步的回调函数需要操作（add timer等）
///初始化有wakeup_fd，timer_fd，acceptor_fd
EventLoop::EventLoop()
        : looping_(false),
          quit_(false),
          threadId_(CurrentThread::tid()),
//          poller_(new Poller(this)),
          poller_(new EPoller(this)),
          timerQueue_(new TimerQueue(this)),
          wakeupFd_(createEventfd()),
          wakeupChannel_(new Channel(this, wakeupFd_))
{
    LOG_TRACE << "EventLoop created " << this << " in thread " << threadId_;
    if(t_loopInThisThread)
    {
        LOG_FATAL << "Another EventLoop " << t_loopInThisThread
                  << " exists in this thread " << threadId_;
    }
    else
    {
        t_loopInThisThread = this;
    }
    wakeupChannel_->setReadCallback(
            std::bind(&EventLoop::handleRead, this));
    // we are always reading the wakeupfd
    wakeupChannel_->enableReading();

}

///用了unique_ptr自动释放资源
EventLoop::~EventLoop()
{
    assert(!looping_);
    ::close(wakeupFd_);
    t_loopInThisThread = NULL;
}

void EventLoop::loop()
{
    assert(!looping_);
    assertInLoopThread();
    looping_ = true;
    quit_ = false;

    while (!quit_)
    {
        activeChannels_.clear();
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        for (const auto& channelptr : activeChannels_)
        {
            channelptr->handleEvent(pollReturnTime_);
        }
        doPendingFunctors();
    }

    LOG_TRACE << "EventLoop " << this << " stop looping";
    looping_ = false;
}


///非本线程则唤醒
void EventLoop::quit()
{
    quit_ = true;
    if (!isInLoopThread())
    {
        wakeup();
    }
}


///本线程则直接调用注册函数@c cb，非本线程则将函数加入
///到待完成队列中
///run_inLoop就像一个任务队列一样，有什么需要执行的函数就往里面扔，然后就可以执行
void EventLoop::runInLoop(const Functor& cb)
{
    if (isInLoopThread())
    {
        cb();
    }
    else
    {
        queueInLoop(cb);
    }
}

///待执行函数队列插入函数，线程安全要加锁
///非本线程则必须唤醒
///本线程在执行函数队列是也需要唤醒（因为即将进入睡眠）
void EventLoop::queueInLoop(const Functor& cb)
{
    {
        MutexLockGuard lock(mutex_);
        pendingFunctors_.push_back(cb);
    }

    if (!isInLoopThread() || callingPendingFunctors_)
    {
        wakeup();
    }
}

///三个定时器功能，异步执行回调函数
TimerId EventLoop::runAt(const Timestamp& time, const TimerCallback& cb)
{
    return timerQueue_->addTimer(cb, time, 0.0);
}

TimerId EventLoop::runAfter(double delay, const TimerCallback& cb)
{
    Timestamp time(addTime(Timestamp::now(), delay));
    return runAt(time, cb);
}

TimerId EventLoop::runEvery(double interval, const TimerCallback& cb)
{
    Timestamp time(addTime(Timestamp::now(), interval));
    return timerQueue_->addTimer(cb, time, interval);
}

void EventLoop::cancel(TimerId timerId)
{
    return timerQueue_->cancel(timerId);
}

///线程安全地更新channel
void EventLoop::updateChannel(Channel* channel)
{
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    poller_->updateChannel(channel);
}

///eventloop删除channel中介
void EventLoop::removeChannel(Channel* channel)
{
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    poller_->removeChannel(channel);
}

void EventLoop::abortNotInLoopThread()
{
    LOG_FATAL << "EventLoop::abortNotInLoopThread - EventLoop " << this
              << " was created in threadId_ = " << threadId_
              << ", current thread id = " <<  CurrentThread::tid();
}

///写eventfd
void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = ::write(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
    }
}

///读event_fd，水平触发必须读，否则一直
void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = ::read(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
    }
}

///注意加锁区域
///设置工作标记，逐个执行
void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        MutexLockGuard lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for(const auto& func : functors)
    {
        func();
    }

    callingPendingFunctors_ = false;
}
