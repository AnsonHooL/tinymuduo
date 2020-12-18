//
// Created by lenovo on 2020/12/9.
//

#ifndef TINYMUDUO_TIMER_H
#define TINYMUDUO_TIMER_H

#include "noncopyable.h"
#include "Atomic.h"
#include "Timestamp.h"
#include "Callbacks.h"

class Timer : noncopyable
{
public:
    Timer(const muduo::TimerCallback& cb, muduo::Timestamp when, double interval)
    : callback_(cb),
      expiration_(when),
      interval_(interval),
      repeat_(interval > 0.0),
      sequence_(s_numCreated_.incrementAndGet())
    { }

    void run() const
    {
        if(callback_)
            callback_();
    }

    muduo::Timestamp expiration() const  { return expiration_; }
    bool repeat() const { return repeat_; };
    int64_t sequence() const { return sequence_; }

    void restart(muduo::Timestamp now);

private:
    const muduo::TimerCallback callback_; ///定时器回调函数
    muduo::Timestamp expiration_; ///定时器过期时间
    const double interval_; ///重复定时间隔
    const bool repeat_; ///是否重复定时器
    const int64_t sequence_; ///定时器序号，用来分辨地址相同，顺序不同的定时器

    static muduo::AtomicInt64 s_numCreated_;

};



#endif //TINYMUDUO_TIMER_H
