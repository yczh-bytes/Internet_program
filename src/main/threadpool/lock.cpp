#include "lock.h"
//锁类
locker::locker()
{
    if(pthread_mutex_init(&m_mutex,NULL)!=0)
    {
        throw std::runtime_error("pthread_mutex_init");
    }
}


bool locker::lock()
{
   return pthread_mutex_lock(&m_mutex);
}

bool locker::unlock()
{
    return pthread_mutex_unlock(&m_mutex);
}

//信号量类
sem::sem()
{
    if(sem_init(&m_sem,0,0)!=0)
    {
      throw std::runtime_error("sem_init");
    }
}

sem::~sem()
{
    sem_destroy(&m_sem);
}

bool sem::wait()
{
    return sem_wait(&m_sem);
}

bool sem::post()
{
    return sem_post(&m_sem);
}

//条件变量类
condition::condition()
{
    if(pthread_cond_init(&m_cond,NULL)!=0)
    {
        throw std::runtime_error("pthread_cond_init");
    }
}

condition::~condition()
{
    pthread_cond_destroy(&m_cond);
}

bool condition::wait(pthread_mutex_t* mutex)
{
    return pthread_cond_wait(&m_cond,mutex);
}

bool condition::waittime(pthread_mutex_t* mutex, struct timespec timeout)
{
    return pthread_cond_timedwait(&m_cond,mutex,&timeout);
}

bool condition::signal()
{
    return pthread_cond_signal(&m_cond);
}