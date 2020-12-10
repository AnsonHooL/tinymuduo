//
// Created by lenovo on 2020/12/9.
//

#ifndef TINYMUDUO_CALLBACKS_H
#define TINYMUDUO_CALLBACKS_H


#include <functional>
#include <memory>

#include "Timestamp.h"

namespace muduo
{

// All client visible callbacks go here.

    typedef std::function<void()> TimerCallback;

}

#endif //TINYMUDUO_CALLBACKS_H
