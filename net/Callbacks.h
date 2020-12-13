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

    class TcpConnection;
    typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;

    typedef std::function<void()> TimerCallback;
    typedef std::function<void (const TcpConnectionPtr&)> ConnectionCallback;
    typedef std::function<void (const TcpConnectionPtr&,
                                  const char* data,
                                  ssize_t len)> MessageCallback;
    typedef std::function<void (const TcpConnectionPtr&)> CloseCallback;

}

#endif //TINYMUDUO_CALLBACKS_H
