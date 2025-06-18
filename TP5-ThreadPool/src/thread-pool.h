#ifndef _thread_pool_
#define _thread_pool_

#include <cstddef>
#include <functional>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include "Semaphore.h"

using namespace std;

struct worker_t
{
  thread ts;
  function<void(void)> thunk;
  bool available;
  Semaphore semaphore;
  size_t id;
  worker_t() : thunk(), available(true), semaphore(0), id(0) {}
};

class ThreadPool
{
public:
  ThreadPool(size_t numThreads);
  void schedule(const function<void(void)> &thunk);
  void wait();
  void shutdown();
  ~ThreadPool();
  ThreadPool(const ThreadPool &original) = delete;
  ThreadPool &operator=(const ThreadPool &rhs) = delete;

private:
  void worker(size_t id);
  void dispatcher();
  thread dt;
  deque<worker_t> wts;
  queue<function<void(void)>> tasks;
  Semaphore tasksAvailable;
  Semaphore workersAvailable;
  mutex queueLock;

  condition_variable cvCompleted;
  mutex completedMutex;
  atomic<size_t> pendingTasks;
  atomic<bool> done;
};

#endif