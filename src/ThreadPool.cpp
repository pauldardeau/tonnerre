// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#include <cstdio>

#include "ThreadPool.h"
#include "ThreadPoolQueue.h"
#include "Logger.h"

//******************************************************************************

ThreadPool::ThreadPool(int numberWorkers) noexcept :
   ThreadPool(ThreadingFactory::getThreadingFactory(), numberWorkers)
{
}

//******************************************************************************

ThreadPool::ThreadPool(std::shared_ptr<ThreadingFactory> threadingFactory,
                       int numberWorkers) noexcept :
   m_threadingFactory(threadingFactory),
   m_queue(m_threadingFactory),
   m_workerCount(numberWorkers),
   m_workersCreated(0),
   m_isRunning(false)
{
   Logger::logInstanceCreate("ThreadPool");
}

//******************************************************************************

ThreadPool::~ThreadPool() noexcept
{
   Logger::logInstanceDestroy("ThreadPool");

   if (m_isRunning) {
      stop();
   }
   
   m_listWorkers.clear();
}

//******************************************************************************

bool ThreadPool::start() noexcept
{
   for (int i = 0; i < m_workerCount; ++i) {
      ++m_workersCreated;
      std::shared_ptr<ThreadPoolWorker> worker(
         new ThreadPoolWorker(m_threadingFactory, m_queue, m_workersCreated));
      worker->start();
      m_listWorkers.push_back(std::move(worker));
   }

   m_isRunning = true;
   
   return true;
}

//******************************************************************************

bool ThreadPool::stop() noexcept
{
   m_queue.shutDown();
   
   for (std::shared_ptr<ThreadPoolWorker>& worker : m_listWorkers) {
      worker->stop();
   }
   
   m_isRunning = false;
   
   return true;
}

//******************************************************************************

bool ThreadPool::addRequest(std::shared_ptr<Runnable> runnableRequest) noexcept
{
   if (!m_isRunning) {
      return false;
   }
   
   m_queue.addRequest(runnableRequest);
   
   return true;
}

//******************************************************************************

std::shared_ptr<Thread> ThreadPool::createThreadWithRunnable(std::shared_ptr<Runnable> runnable) noexcept
{
   return m_threadingFactory->createThread(runnable);
}

//******************************************************************************

int ThreadPool::getNumberWorkers() const noexcept
{
	return m_workerCount;
}

//******************************************************************************

void ThreadPool::addWorkers(int numberNewWorkers) noexcept
{
	adjustNumberWorkers(numberNewWorkers);
}

//******************************************************************************

void ThreadPool::removeWorkers(int numberWorkersToRemove) noexcept
{
	adjustNumberWorkers(-numberWorkersToRemove);
}

//******************************************************************************

void ThreadPool::adjustNumberWorkers(int numberToAddOrDelete) noexcept
{
   if (numberToAddOrDelete > 0) {   // adding?
      
		const int newNumberWorkers = m_workerCount + numberToAddOrDelete;
		
	   for (int i = m_workerCount; i < newNumberWorkers; ++i) {
         ++m_workersCreated;
         ++m_workerCount;
         std::unique_ptr<ThreadPoolWorker> worker(
            new ThreadPoolWorker(m_threadingFactory, m_queue, m_workersCreated));
         
         if (m_isRunning) {
            worker->start();
         }
         
	      m_listWorkers.push_back(std::move(worker));
	   }
   } else if (numberToAddOrDelete < 0) {  // removing?
      if (m_isRunning) {
	      // tell thread to shut down
      }
   }
}

//******************************************************************************
