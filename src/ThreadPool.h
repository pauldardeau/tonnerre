// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <list>
#include <memory>

#include "Thread.h"
#include "ThreadPoolQueue.h"
#include "ThreadPoolWorker.h"
#include "ThreadPoolDispatcher.h"
#include "ThreadingFactory.h"

class Runnable;


/**
 * ThreadPool is an interface (abstract base class) for thread pools.
 */
class ThreadPool : public ThreadPoolDispatcher
{
public:
   /**
    *
    * @param numberWorkers
    */
   explicit ThreadPool(int numberWorkers) noexcept;
   
   /**
    *
    * @param threadingFactory
    * @param numberWorkers
    */
   ThreadPool(std::shared_ptr<ThreadingFactory> threadingFactory, int numberWorkers) noexcept;

   /**
    * Destructor
    */
   ~ThreadPool() noexcept;
   
   // ThreadPoolDispatcher
   /**
    *
    * @return
    */
   virtual bool start() noexcept override;
   
   /**
    *
    * @return
    */
   virtual bool stop() noexcept override;
   
   /**
    *
    * @param runnableRequest
    * @return
    * @see Runnable()
    */
   virtual bool addRequest(std::shared_ptr<Runnable> runnableRequest) noexcept override;
   
   /**
    *
    * @param runnable
    * @return
    * @see Runnable()
    * @see Thread()
    */
   virtual std::shared_ptr<Thread> createThreadWithRunnable(std::shared_ptr<Runnable> runnable) noexcept;
   
   /**
    *
    * @return
    */
   int getNumberWorkers() const noexcept;
   
   /**
    *
    * @param numberNewWorkers
    */
   void addWorkers(int numberNewWorkers) noexcept;
   
   /**
    *
    * @param numberWorkersToRemove
    */
   void removeWorkers(int numberWorkersToRemove) noexcept;
   
   ThreadPool(const ThreadPool&) = delete;
   ThreadPool(ThreadPool&&) = delete;
   ThreadPool& operator=(const ThreadPool&) = delete;
   ThreadPool& operator=(ThreadPool&&) = delete;
   
protected:
   /**
    *
    * @param numberToAddOrDelete
    */
   void adjustNumberWorkers(int numberToAddOrDelete) noexcept;
   
private:
   std::shared_ptr<ThreadingFactory> m_threadingFactory;
   std::list<std::shared_ptr<ThreadPoolWorker>> m_listWorkers;
   ThreadPoolQueue m_queue;
   int m_workerCount;
   int m_workersCreated;
   bool m_isRunning;
};

#endif
