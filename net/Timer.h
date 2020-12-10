//
// Created by lenovo on 2020/12/9.
//

#ifndef TINYMUDUO_TIMER_H
#define TINYMUDUO_TIMER_H

#include <noncopyable.h>

#include "Timestamp.h"
#include "Callbacks.h"

class Timer : noncopyable
{
public:
    Timer(const muduo::TimerCallback& cb, muduo::Timestamp when, double interval)
    : callback_(cb),
      expiration_(when),
      interval_(interval),
      repeat_(interval > 0.0)
    { }

    void run() const
    {
        if(callback_)
            callback_();
    }

    muduo::Timestamp expiration() const  { return expiration_; }
    bool repeat() const { return repeat_; }

    void restart(muduo::Timestamp now);

private:
    const muduo::TimerCallback callback_; //定时器回调函数
    muduo::Timestamp expiration_; //定时器过期时间
    const double interval_; //重复定时间隔
    const bool repeat_; //是否重复定时器

};



#endif //TINYMUDUO_TIMER_H
