#include "thread_worker.hpp"

#include <iostream>

// TODO: Delete it once thread worker is stable
// class API {
//   public:
//    API() { std::cout << "<<create>> API" << std::endl; };
//    void task(std::string payload) {
//       std::cout << "API.task: " << payload << std::endl;
//       std::this_thread::sleep_for(std::chrono::seconds(1));
//    }
// };

ThreadWorker::ThreadWorker()
    : worker_mtx(std::make_shared<std::mutex>()),
      queue_update(std::make_shared<std::condition_variable>()),
      queue(std::make_shared<task_list>()),
      worker(nullptr){
          //    std::cout << "<<create>> Worker" << std::endl;
      };

ThreadWorker::~ThreadWorker() {
   //    std::cout << "<<DELETE>> worker" << std::endl;
   stop();
}

void ThreadWorker::thread_loop() {
   //    unsigned int i = 0;
   //    std::cout << "<<create>> ThreadWorker LOOP" << std::endl;

   while (worker_enabled) {
      async_task task;
      bool has_task = false;
      {  // mutex guard
         unique_lock mtx(*worker_mtx);
         if (queue->empty()) {
            queue_update->wait(mtx, [=]() { return !worker_enabled or !queue->empty(); });
         }

         if (queue->size() > 0) {
            task = queue->front();
            queue->pop_front();
            has_task = true;
         }
      }  // mutex guard end

      if (has_task) {
         std::cout << "<<<worker TASK: execute start>>>" << std::endl;
         task();
         std::cout << "<<<worker TASK: execute finished>>>" << std::endl;

      }
      //   std::cout << "<<loop>> thread:" << ++i << "  queue_size=" << queue->size()
      //             << "   is_needed=" << worker_enabled << "   has_task=" << has_task << std::endl;
   }
}

void ThreadWorker::execute_async_task(async_task task) {
   //    std::cout << "<add task>" << std::endl;
   guard_lock lock(*worker_mtx);
   queue->push_back(task);
   queue_update->notify_one();
}

void ThreadWorker::start() {
   //    std::cout << "<<start>> the loop" << std::endl;
   worker_enabled = true;
   worker = std::make_shared<std::thread>([=]() { thread_loop(); });
}

void ThreadWorker::stop() {
   //    std::cout << "<<stop>> the loop" << std::endl;
   if (nullptr == worker) return;
   {
      guard_lock lock(*worker_mtx);
      worker_enabled = false;
      queue_update->notify_all();
   }
   worker->join();
   worker = nullptr;
}

void ThreadWorker::remove_pending_tasks() {
   //    std::cout << "<<remove>> all tasks" << std::endl;
   guard_lock lock(*worker_mtx);
   queue->clear();
}
