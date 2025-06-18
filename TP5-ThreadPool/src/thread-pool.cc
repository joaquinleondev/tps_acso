#include "thread-pool.h"
#include <iostream>

ThreadPool::ThreadPool(size_t numThreads)
    : tasksAvailable(0),
      workersAvailable((int)numThreads),
      pendingTasks(0),
      done(false)
{
    // Inicializa los workers
    wts.resize(numThreads);
    for (size_t i = 0; i < numThreads; ++i)
    {
        wts[i].id = i;
        wts[i].available = true;
        wts[i].ts = thread(&ThreadPool::worker, this, i);
    }
    dt = thread(&ThreadPool::dispatcher, this);
}

void ThreadPool::schedule(const function<void(void)> &thunk)
{
    if (!thunk)
        throw runtime_error("ThreadPool: cannot schedule nullptr task");
    if (done.load())
    {
        throw runtime_error("ThreadPool is shutting down, cannot schedule new tasks.");
    }
    {
        lock_guard<mutex> lk(queueLock);
        tasks.push(thunk);
        pendingTasks.fetch_add(1, memory_order_relaxed);
    }
    tasksAvailable.signal();
}

void ThreadPool::shutdown()
{
    // 1) espera a que terminen las tareas
    wait();
    // 2) indica shutdown y despierta dispatcher
    {
        lock_guard<mutex> lk(queueLock);
        done.store(true, memory_order_release);
    }
    tasksAvailable.signal();
    if (dt.joinable())
        dt.join();
    // 3) despierta y une a todos los workers
    for (auto &w : wts)
        w.semaphore.signal();
    for (auto &w : wts)
        if (w.ts.joinable())
            w.ts.join();
}

void ThreadPool::dispatcher()
{
    while (true)
    {
        tasksAvailable.wait();
        {
            lock_guard<mutex> lk(queueLock);
            if (done.load(memory_order_acquire) && tasks.empty())
                break;
        }
        workersAvailable.wait();
        function<void(void)> task;
        size_t workerId = 0;
        {
            lock_guard<mutex> lk(queueLock);
            task = tasks.front();
            tasks.pop();
            for (size_t i = 0; i < wts.size(); ++i)
            {
                if (wts[i].available)
                {
                    wts[i].available = false;
                    workerId = i;
                    break;
                }
            }
        }
        wts[workerId].thunk = task;
        wts[workerId].semaphore.signal();
    }
}

void ThreadPool::worker(size_t id)
{
    while (true)
    {
        wts[id].semaphore.wait();
        if (done.load(memory_order_acquire))
            break;
        try
        {
            wts[id].thunk();
        }
        catch (...)
        {
        }
        {
            lock_guard<mutex> lk(queueLock);
            wts[id].available = true;
        }
        workersAvailable.signal();
        if (pendingTasks.fetch_sub(1, memory_order_acq_rel) == 1)
        {
            lock_guard<mutex> lk(completedMutex);
            cvCompleted.notify_all();
        }
    }
}

void ThreadPool::wait()
{
    unique_lock<mutex> lk(completedMutex);
    cvCompleted.wait(lk, [this]()
                     { return pendingTasks.load(memory_order_acquire) == 0; });
}

ThreadPool::~ThreadPool()
{
    shutdown();
}
