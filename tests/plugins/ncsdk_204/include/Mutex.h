#ifndef __HDDL_UTILS_MUTEX_H__
#define __HDDL_UTILS_MUTEX_H__

#include <mutex>

class Mutex
{
public:
    Mutex() {}
    ~Mutex() {}

    void lock() { m_mutex.lock(); }
    void unlock() { m_mutex.unlock(); }
    bool tryLock() { return m_mutex.try_lock(); }

private:
    std::mutex m_mutex;

    friend class Condition;
    friend class AutoMutex;
};

class AutoMutex
{
public:
    AutoMutex(Mutex* mutex) : m_mutex(mutex) {
        m_mutex->m_mutex.lock();
    };

    AutoMutex(Mutex& mutex) : m_mutex(&mutex) {
        m_mutex->m_mutex.lock();
    };

    ~AutoMutex() {
        m_mutex->m_mutex.unlock();
    };

private:
    Mutex* m_mutex;
};

#endif
