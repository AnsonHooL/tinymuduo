//
// Created by lenovo on 2020/12/9.
//

#define __STDC_LIMIT_MACROS

#include "TimerQueue.h"

#include "Logging.h"
#include "EventLoop.h"
#include "Timer.h"
#include "TimerId.h"

#include <functional>

#include <sys/timerfd.h>


namespace detail
{
///创建时钟fd
int createTimerfd()
{
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC, //不受用户修改影响
                                   TFD_NONBLOCK | TFD_CLOEXEC); //非阻塞，子进程fork自动close该fd
    if (timerfd < 0)
    {
        LOG_SYSFATAL << "Failed in timerfd_create";
    }
    return timerfd;
}

///计算定时器过期时间
struct timespec howMuchTimeFromNow(Timestamp when)
{
    int64_t microseconds = when.microSecondsSinceEpoch()
                           - Timestamp::now().microSecondsSinceEpoch();
    if (microseconds < 100)
    {
        microseconds = 100;
    }
    struct timespec ts;
    ts.tv_sec = static_cast<time_t>(
            microseconds / Timestamp::kMicroSecondsPerSecond);
    ts.tv_nsec = static_cast<long>(
            (microseconds % Timestamp::kMicroSecondsPerSecond) * 1000);
    return ts;
}

///timer fd可读时读取
void readTimerfd(int timerfd, Timestamp now)
{
    uint64_t howmany;
    ssize_t n = ::read(timerfd, &howmany, sizeof howmany);
    LOG_TRACE << "TimerQueue::handleRead() " << howmany << " at " << now.toString();
    if (n != sizeof howmany)
    {
        LOG_ERROR << "TimerQueue::handleRead() reads " << n << " bytes instead of 8";
    }
}

///重置timer fd过期时间
void resetTimerfd(int timerfd, Timestamp expiration)
{
    // wake up loop by timerfd_settime()
    struct itimerspec newValue;
    struct itimerspec oldValue;
    bzero(&newValue, sizeof newValue);
    bzero(&oldValue, sizeof oldValue);
    newValue.it_value = howMuchTimeFromNow(expiration);
    int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);
    if (ret)
    {
        LOG_SYSERR << "timerfd_settime()";
    }
}

}

using namespace ::detail;

///初始化定时器队列
///初始化列表，按声明顺序初始化，不漏掉任何一个
///定时器到时事件 ---->>>> fd可读事件，做的是剔除定时器
///定时器队列维护一个定时器fd，并设置定时时间，令channel可读
TimerQueue::TimerQueue(EventLoop* loop)
        : loop_(loop),
          timerfd_(createTimerfd()),
          timerfdChannel_(loop, timerfd_),
          timers_()
{
    timerfdChannel_.setReadCallback(
            std::bind(&TimerQueue::handleRead, this));
    // we are always reading the timerfd, we disarm it with timerfd_settime.
    timerfdChannel_.enableReading();
}

TimerQueue::~TimerQueue()
{
    ::close(timerfd_);
    // do not remove channel, since we're in EventLoop::dtor();
    for (TimerList::iterator it = timers_.begin();
         it != timers_.end(); ++it)
    {
        delete it->second;
    }
}



///线程安全的往事件循环里面异步加入定时器
///使用run in loop
TimerId TimerQueue::addTimer(const TimerCallback& cb,
                             Timestamp when,
                             double interval)
{
    Timer* timer = new Timer(cb, when, interval);
    loop_->runInLoop(
            std::bind(&TimerQueue::addTimerInLoop,this,timer));
    return TimerId(timer);
}

///增加一个定时器，插入到有序的定时器set中，并判断其是否是最早的定时器
///如果是，则更新timer_fd
void TimerQueue::addTimerInLoop(Timer* timer)
{
    loop_->assertInLoopThread();
    bool earliestChanged = insert(timer);

    if (earliestChanged)
    {
        resetTimerfd(timerfd_, timer->expiration());
    }
}


///timer_fd读事件发生，将set中过期的定时器都提取出来
///执行定时器的任务
void TimerQueue::handleRead()
{
    loop_->assertInLoopThread();
    Timestamp now(Timestamp::now());
    readTimerfd(timerfd_, now);

    std::vector<Entry> expired = getExpired(now);

    // safe to callback outside critical section
    for (std::vector<Entry>::iterator it = expired.begin();
         it != expired.end(); ++it)
    {
        it->second->run();
    }

    reset(expired, now);
}


///lowerbound是指向第一个大于等于value：<now,MAXPTR>，不可能等于，因为地址不可能取到最大64位
///所以it指向的时间戳一定大于now
///set底层是红黑树，红黑树是2，3，4树，有序的
///驱逐出set中所有时间小于it的定时器：back_inserter(expired)，pushback的迭代器
std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now)
{
    std::vector<Entry> expired;
    Entry sentry = std::make_pair(now, reinterpret_cast<Timer*>(UINTPTR_MAX));
    TimerList::iterator it = timers_.lower_bound(sentry);
    assert(it == timers_.end() || now < it->first);
    std::copy(timers_.begin(), it, back_inserter(expired));
    timers_.erase(timers_.begin(), it);

    return expired;
}

///判断驱逐的定时器中哪些是重复任务，并重置定时器
///若是单次定时器则释放资源
///检查定时器队列非空则添重置timer fd超时时间
void TimerQueue::reset(const std::vector<Entry>& expired, Timestamp now)
{
    Timestamp nextExpire;

    for (std::vector<Entry>::const_iterator it = expired.begin();
         it != expired.end(); ++it)
    {
        if (it->second->repeat())
        {
            it->second->restart(now);
            insert(it->second);
        }
        else
        {
            // FIXME move to a free list
            delete it->second;
        }
    }
    if (!timers_.empty())
    {
        nextExpire = timers_.begin()->second->expiration();
    }

    if (nextExpire.valid())
    {
        resetTimerfd(timerfd_, nextExpire);
    }
}


///插入一个定时器，队列为空或者，定时器小于最小的定时器则需要重设fd超时时间
bool TimerQueue::insert(Timer* timer)
{
    bool earliestChanged = false;
    Timestamp when = timer->expiration();
    TimerList::iterator it = timers_.begin();
    if (it == timers_.end() || when < it->first)
    {
        earliestChanged = true;
    }
    std::pair<TimerList::iterator, bool> result =
            timers_.insert(std::make_pair(when, timer));
    assert(result.second);
    return earliestChanged;
}



















