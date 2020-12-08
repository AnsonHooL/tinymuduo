//
// Created by lenovo on 2020/12/8.
//

#ifndef TINYMUDUO_NONCOPYABLE_H
#define TINYMUDUO_NONCOPYABLE_H

class noncopyable
{
public:
    noncopyable() = default;
    ~noncopyable() = default;
    noncopyable(const noncopyable& other) = delete;
    noncopyable& operator=(const noncopyable& other) = delete;
    noncopyable& operator=(const noncopyable&& other) = delete;
};








#endif //TINYMUDUO_NONCOPYABLE_H
