#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include "threadpool.h"

using std::vector;
using std::unique_ptr;

namespace WebServer {

// The main thread loop.
// Each thread waits on the condition variable until signaled.
// Upon waking, threads grab whatever is in the work queue and execute
void* SysiphusLoop(void* thread_pool) {
  ThreadPool* pool = static_cast<ThreadPool*>(thread_pool);
  pthread_mutex_lock(&(pool->q_lock_));
  pool->num_threads_running_++;

  while (!pool->killthreads_) {
    pthread_cond_wait(&(pool->q_cond_), &(pool->q_lock_));
    while (!pool->work_queue_.empty() && !pool->killthreads_) {
      unique_ptr<ThreadPool::Task> nextTask = std::move(pool->work_queue_.front());
      pool->work_queue_.pop_front();
      pthread_mutex_unlock(&(pool->q_lock_));
      nextTask->Run();
      pthread_mutex_lock(&(pool->q_lock_));
    }
  }
  pool->num_threads_running_--;
  pthread_mutex_unlock(&(pool->q_lock_));
  return nullptr;
}

ThreadPool::ThreadPool(int max_threads) : killthreads_(false), num_threads_running_(0) {
  pthread_mutex_init(&q_lock_, nullptr);
  pthread_cond_init(&q_cond_, nullptr);
  thread_array_ = vector<pthread_t>(max_threads);
  pthread_mutex_lock(&q_lock_);
  for (int i = 0; i < max_threads; i++) {
    pthread_create(&(thread_array_[i]),
                    nullptr,
                    &SysiphusLoop,
                    static_cast<void*>(this));
  }
  while (num_threads_running_ != max_threads) {
    pthread_mutex_unlock(&q_lock_);
    sleep(1);  
    pthread_mutex_lock(&q_lock_);
  }
  pthread_mutex_unlock(&q_lock_);
}
  
ThreadPool::~ThreadPool() {
  pthread_mutex_lock(&q_lock_);
  int num_threads = num_threads_running_;
  killthreads_ = true;
  for (int i = 0; i < num_threads; i++) {
    pthread_cond_broadcast(&q_cond_);
    pthread_mutex_unlock(&q_lock_);
    pthread_join(thread_array_[i], nullptr);
    pthread_mutex_lock(&q_lock_);
  }
  pthread_mutex_unlock(&q_lock_);
  while (!work_queue_.empty()) {
    unique_ptr<ThreadPool::Task> nextTask = std::move(work_queue_.front());
    work_queue_.pop_front();
    nextTask->Run();
  }
}

void ThreadPool::Dispatch(std::unique_ptr<Task> task) {
  pthread_mutex_lock(&q_lock_);
  work_queue_.push_back(std::move(task));
  pthread_cond_signal(&q_cond_);
  pthread_mutex_unlock(&q_lock_);
}

void ThreadPool::KillThreads() {
  killthreads_ = true;
  for (size_t i = 0; i < thread_array_.size(); i++) {
    pthread_kill(thread_array_[i], SIGINT);
    pthread_join(thread_array_[i], nullptr);
  }
}

} // end namespace WebServer