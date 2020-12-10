//
// Created by lenovo on 2020/12/9.
//

#ifndef TINYMUDUO_TIMERID_H
#define TINYMUDUO_TIMERID_H

#include "copyable.h"

class Timer;

///
/// An opaque identifier, for canceling Timer.
///
class TimerId : public muduo::copyable
{
public:
    explicit TimerId(Timer* timer)
            : value_(timer)
    {
    }

    // default copy-ctor, dtor and assignment are okay

private:
    Timer* value_;
};


#endif //TINYMUDUO_TIMERID_H
