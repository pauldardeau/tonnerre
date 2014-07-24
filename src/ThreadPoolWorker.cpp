// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#include <exception>

#include "ThreadPoolWorker.h"
#include "ThreadPoolQueue.h"
#include "ThreadingFactory.h"
#include "Thread.h"
#include "BasicException.h"
#include "Logger.h"

//******************************************************************************

ThreadPoolWorker::ThreadPoolWorker(std::shared_ptr<ThreadingFactory> threadingFactory,
                                   ThreadPoolQueue& queue,
                                   int workerId) noexcept :
   m_threadingFactory(threadingFactory),
   m_workerThread(nullptr),
   m_poolQueue(queue),
   m_workerId(workerId),
   m_isRunning(false)
{
   Logger::logInstanceCreate("ThreadPoolWorker");
}

//******************************************************************************

ThreadPoolWorker::~ThreadPoolWorker() noexcept
{
   Logger::logInstanceDestroy("ThreadPoolWorker");
   
}

//******************************************************************************

void ThreadPoolWorker::start() noexcept
{
   if (!m_workerThread) {
      m_workerThread = m_threadingFactory->createThread(shared_from_this());
      if (m_workerThread) {
         m_workerThread->setPoolWorkerStatus(true);
         m_workerThread->setWorkerId(std::to_string(m_workerId));
         m_isRunning = true;
         m_workerThread->start();
      }
   }
}

//******************************************************************************

void ThreadPoolWorker::stop() noexcept
{
   m_isRunning = false;
}

//******************************************************************************

void ThreadPoolWorker::run() noexcept
{
   while (m_isRunning) {
      if (Logger::isLogging(Logger::LogLevel::Debug)) {
         char message[128];
         std::snprintf(message, 128, "poolQueue taking request on thread %d", m_workerId);
         Logger::debug(std::string(message));
      }

      std::shared_ptr<Runnable> runnable = m_poolQueue.takeRequest();
      if (runnable) {
	      // has our thread been notified to shut down?
	      if (!m_workerThread->isAlive()) {
		      // put the request back on the front of the queue
				m_isRunning = false;
				continue;
	      } else {
            
            // mark it
            runnable->setRunByThreadId(m_workerId);
            runnable->setRunByThreadWorkerId(m_workerThread->getWorkerId());
            
            try
            {
               runnable->run();
            }
            catch (const BasicException& be)
            {
               Logger::error("run method of runnable threw exception: " + be.whatString());
            }
            catch (const std::exception& e)
            {
               Logger::error("run method of runnable threw exception: " + std::string(e.what()));
            }
            catch (...)
            {
               Logger::error("run method of runnable threw exception");
            }

            if (Logger::isLogging(Logger::LogLevel::Debug)) {
               char message[128];
               std::snprintf(message, 128, "ending processing request on thread %d", m_workerId);
               Logger::debug(std::string(message));
            }
            
         }
      }
   }
}

//******************************************************************************


