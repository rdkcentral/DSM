#ifndef __THREAD_WORKER_HPP
#define __THREAD_WORKER_HPP

#include <condition_variable>
#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <thread>

using async_task = std::function<void()>;
using unique_lock = std::unique_lock<std::mutex>;
using guard_lock = std::lock_guard<std::mutex>;
using task_list = std::list<async_task>;

class ThreadWorker {
   std::shared_ptr<std::mutex> worker_mtx;
   std::shared_ptr<std::condition_variable> queue_update;
   std::shared_ptr<task_list> queue;
   std::shared_ptr<std::thread> worker;
   bool worker_enabled{true};

   void thread_loop();

  public:
   ThreadWorker();
   ~ThreadWorker();

   void execute_async_task(async_task task);
   void start();
   void stop();
   void remove_pending_tasks();
};

#endif
