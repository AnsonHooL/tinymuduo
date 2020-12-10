//
// Created by lenovo on 2020/12/8.
//

#ifndef TINYMUDUO_NONCOPYABLE_H
#define TINYMUDUO_NONCOPYABLE_H

class noncopyable
{
protected:
    noncopyable() = default;
    ~noncopyable() = default;

private:
    noncopyable(const noncopyable&);
    noncopyable& operator=(const noncopyable&);
    noncopyable& operator=(const noncopyable&&);
};








#endif //TINYMUDUO_NONCOPYABLE_H
