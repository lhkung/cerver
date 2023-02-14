#ifndef THREAD_POOL_H_
#define THREAD_POOL_H_

#include <list>
#include <vector>
#include <memory>

namespace WebServer {

// A thread pool based on POSIX threads.
// To use this class, define a class that derives from [Task]
// and override the Run() method.
class ThreadPool {
  public:
    ThreadPool(int max_threads);
    ~ThreadPool();
    class Task {
      public:
        Task() { }
        virtual ~Task() { }
        virtual void Run() = 0;
    };
    void Dispatch(std::unique_ptr<Task> task);
    void KillThreads();
    pthread_mutex_t q_lock_;
    pthread_cond_t  q_cond_;
    std::list<std::unique_ptr<Task> > work_queue_;
    bool killthreads_;
    int num_threads_running_;

  private:
    std::vector<pthread_t> thread_array_;
};

} // namespace WebServer

#endif