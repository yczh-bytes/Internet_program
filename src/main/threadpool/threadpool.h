#pragma once
#include "lock.h"
#include <list>
#include <cstdio>
#include <exception>
//线程池类
template<typename T>
class threadpool
{
    public:
    //构造函数
    threadpool(int thread_num=8,int max_requests=10000);
    //析构函数
    ~threadpool();
    //添加任务
    bool append(T* task);
    private:
    //线程数量
    int m_thread_num;
    //线程指针数组
    pthread_t* m_threads;
    //任务队列
    std::list<T*> m_task_queue;
    //最大请求数
    int m_max_requests;
    //任务队列锁
    locker m_queue_lock;
    //信号量判断队列是否有任务
    sem m_sem;
    //条件变量判断队列是否有任务
    condition m_cond;
    //判断任务是否结束
    bool m_stop;
    //静态线程函数
    static void* thread_func(void* arg);
    //执行任务
    void run();
};

