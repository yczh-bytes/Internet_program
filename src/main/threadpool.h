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
    unsigned int m_max_requests;
    //任务队列锁
    locker m_queue_lock;
    //信号量判断队列是否有任务
    sem m_sem;
    //判断任务是否结束
    bool m_stop;
    //静态线程函数
    static void* thread_func(void* arg);
    //执行任务
    void run();
};

template<typename T>
threadpool<T>::threadpool(int thread_num,int max_requests):
m_thread_num(thread_num),m_threads(NULL),m_max_requests(max_requests),m_queue_lock(),m_sem(),m_stop(false)
{
    //检查线程数量和最大请求数是否有效
    if(m_thread_num<=0||m_max_requests<=0)
    {
        throw std::runtime_error("threadpool");
    }
    //创建线程数组
    m_threads=new pthread_t[m_thread_num];
    //检查线程数组是否创建成功
    if(!m_threads)
    {
        throw std::runtime_error("threadpool");
    }

    //循环创建线程
    for(int i = 0;i<m_thread_num;i++)
    {
        printf("create thread %d\n",i);
        //创建线程
        //此处传入this指针的目的是为了静态线程函数可以访问成员变量
        
        if(pthread_create(&m_threads[i],NULL,thread_func,this)!=0)
        {
             delete [] m_threads;
            m_threads = NULL;
            throw std::runtime_error("threadpool");
        }
    }

    //设置线程分离属性
    for(int i = 0;i<m_thread_num;i++)
    {
       if(pthread_detach(m_threads[i]))
       {
        throw std::runtime_error("threadpool");
        delete [] m_threads;
        m_threads = NULL;
       }
    }
}

template<typename T>
threadpool<T>::~threadpool()
{
    m_stop = true;
    //唤醒所有等待的线程，让它们退出
    for(int i = 0;i<m_thread_num;i++)
    {
        m_sem.post();
    }
    delete [] m_threads;
    m_threads = NULL;
}

template<typename T>
bool threadpool<T>::append(T* task)
{
    //获取锁
    m_queue_lock.lock();
    //判断是否还有空位
    if(m_task_queue.size()>=m_max_requests)
    {
        //解锁并返回加入失败
        m_queue_lock.unlock();
        return false;
    }
    //将任务加入队列
    m_task_queue.push_back(task);
    //解锁，信号量加一
    m_queue_lock.unlock();
    m_sem.post();

    return true;
}

//实现线程函数
template<typename T>
void* threadpool<T>::thread_func(void* arg)
{
    threadpool<T>* pool = (threadpool<T>*)arg;
    pool->run();

    return NULL;
}

//实现run函数
template<typename T>
void threadpool<T>::run()
{
    while(!m_stop)
    {
        //等待信号量
        m_sem.wait();
        if(m_stop)
        {
            break;
        }
        //获取锁
        m_queue_lock.lock();
        //判断队列是否为空
        if(m_task_queue.empty())
        {
           //解锁
           m_queue_lock.unlock();
           continue;
               }
         //从队列中取出任务
            T* task = m_task_queue.front();
            //从队列中删除任务
            m_task_queue.pop_front();
            //解锁
            m_queue_lock.unlock();
            //执行任务
            task->process();
    }
}