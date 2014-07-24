// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef THREADPOOLWORKER_H
#define THREADPOOLWORKER_H

#include <memory>

#include "Runnable.h"

class ThreadingFactory;
class ThreadPoolQueue;
class Thread;


/*!
 * ThreadPoolWorker provides the logic of pulling work off of a ThreadPoolQueue
 * and processing and then repeating. It's meant to be used within a ThreadPool.
 */
class ThreadPoolWorker : public Runnable
{
   public:
      ThreadPoolWorker(std::shared_ptr<ThreadingFactory> threadingFactory,
                       ThreadPoolQueue& queue,
                       int workerId) noexcept;
      ~ThreadPoolWorker() noexcept;

      void start() noexcept;
      void stop() noexcept;
      void run() noexcept override;

      // disallow copies
      ThreadPoolWorker(const ThreadPoolWorker&);
      ThreadPoolWorker(ThreadPoolWorker&&);
      ThreadPoolWorker& operator=(const ThreadPoolWorker&);
      ThreadPoolWorker& operator=(ThreadPoolWorker&&);

   private:
      std::shared_ptr<ThreadingFactory> m_threadingFactory;
      std::shared_ptr<Thread> m_workerThread;
      ThreadPoolQueue& m_poolQueue;
      int m_workerId;
      bool m_isRunning;
};

#endif

