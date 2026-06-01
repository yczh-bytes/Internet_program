#pragma once
#include <pthread.h>
#include <semaphore.h>
#include <stdexcept>
//锁类
class locker
{
    public:
    locker();
    //创建一把锁
    bool lock();
    //解锁
    bool unlock();
    private:
    //创建一把锁
    pthread_mutex_t m_mutex;
};
//信号量类
class sem
{
    public:
    //创建一个信号量
    sem();
    //信号量减一
    bool wait();
    //信号量加一
    bool post();
    //销毁信号量
    ~sem();
    private:
    //创建一个信号量
    sem_t m_sem;
};
//条件变量类
class condition
{
    public:
    //创建一个条件变量
    condition();
    //销毁条件变量
    ~condition();
    //等待条件变量
    bool wait(pthread_mutex_t* mutex);
    //有时间限制的条件变量等待
    bool waittime(pthread_mutex_t* mutex, struct timespec timeout);
    //条件变量加一
    bool signal();
       private:
    //创建一个条件变量
    pthread_cond_t m_cond;
};

