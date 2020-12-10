//
// Created by lenovo on 2020/12/9.
//

#ifndef TINYMUDUO_TIMERQUEUE_H
#define TINYMUDUO_TIMERQUEUE_H

#include <set>
#include <vector>

#include <noncopyable.h>

#include "Timestamp.h"
//#include "thread/Mutex.h"
#include "Callbacks.h"
#include "Channel.h"

class EventLoop;
class Timer;
class TimerId;



class TimerQueue : noncopyable
{
public:
    TimerQueue(EventLoop* loop);
    ~TimerQueue();

    ///
    /// Schedules the callback to be run at given time,
    /// repeats if @c interval > 0.0.
    ///
    /// Must be thread safe. Usually be called from other threads.
    /// 被其他线程调用runat()时候就要保证线程安全，这里不用锁机制，用函数队列
    /// EventLoop用run in loop后可以解决线程安全的问题
    TimerId addTimer(const muduo::TimerCallback& cb,
                     muduo::Timestamp when,
                     double interval);

    // void cancel(TimerId timerId);

private:

    // FIXME: use unique_ptr<Timer> instead of raw pointers.
    typedef std::pair<muduo::Timestamp, Timer*> Entry;//用同时到期的定时器，所以用时间戳+地址找到唯一定时器
    typedef std::set<Entry> TimerList;

    void addTimerInLoop(Timer* timer);
    // called when timerfd alarms
    void handleRead();
    // move out all expired timers
    std::vector<Entry> getExpired(muduo::Timestamp now); //定时器到时
    void reset(const std::vector<Entry>& expired, muduo::Timestamp now); //重复定时器

    bool insert(Timer* timer); //定时器队列理添加一个新的定时器

    EventLoop* loop_; //所属的事件循环
    const int timerfd_; //用linux提供的接口实现，定时事件变成timerfd可读事件
    Channel timerfdChannel_;
    // Timer list sorted by expiration
    TimerList timers_; //定时器列表，set是有序的，按时间戳由小到大排序
};




















#endif //TINYMUDUO_TIMERQUEUE_H
