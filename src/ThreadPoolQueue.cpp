// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#include <cstdio>
#include <cstdlib>

#include "ThreadPoolQueue.h"
#include "ThreadingFactory.h"
#include "ConditionVariable.h"
#include "MutexLock.h"
#include "Logger.h"
#include "BasicException.h"

//******************************************************************************

ThreadPoolQueue::ThreadPoolQueue(std::shared_ptr<ThreadingFactory> threadingFactory) noexcept :
   m_threadingFactory(threadingFactory),
   m_mutex(nullptr),
   m_condQueueEmpty(nullptr),
   m_condQueueNotEmpty(nullptr),
   m_isInitialized(false),
   m_isRunning(false)
{
   Logger::logInstanceCreate("ThreadPoolQueue");

   try {
      m_mutex = m_threadingFactory->createMutex();
      m_condQueueEmpty = m_threadingFactory->createConditionVariable();
      m_condQueueNotEmpty = m_threadingFactory->createConditionVariable();
         
      if (m_mutex && m_condQueueEmpty && m_condQueueNotEmpty) {
         m_isInitialized = true;
         m_isRunning = true;
      }
   }
   catch (const BasicException& be)
   {
      Logger::error("exception setting up thread pool queue: " + be.whatString());
   }
   catch (const std::exception& e)
   {
      Logger::error("exception setting up thread pool queue: " + std::string(e.what()));
   }
   catch (...)
   {
      Logger::error("unknown exception setting up thread pool queue");
   }
}

//******************************************************************************

ThreadPoolQueue::~ThreadPoolQueue() noexcept
{
   Logger::logInstanceDestroy("ThreadPoolQueue");
   
   m_isRunning = false;
}

//******************************************************************************

bool ThreadPoolQueue::addRequest(std::shared_ptr<Runnable> runnableRequest) noexcept
{
   if (!m_isInitialized) {
      Logger::log(Logger::LogLevel::Warning, "ThreadPoolQueue::addRequest queue not initialized");
      return false;
   }

   if (!runnableRequest) {
      Logger::log(Logger::LogLevel::Warning, "ThreadPoolQueue::addRequest rejecting NULL request");
      return false;
   }
   
   //MutexLock lock(*m_mutex);
   
   if (!m_isRunning) {
      Logger::log(Logger::LogLevel::Warning, "ThreadPoolQueue::addRequest rejecting request, queue is shutting down");
      return false;
   }
   
   if (!m_mutex->haveValidMutex()) {
      Logger::error("don't have valid mutex in addRequest");
      exit(1);
   }

   if (!m_mutex->lock()) {
      Logger::error("unable to lock mutex in addRequest");
      exit(1);
   }
   
   Logger::log(Logger::LogLevel::Debug, "ThreadPoolQueue::addRequest accepting request");
   
   const bool wasEmpty = m_queue.empty();
   
   // add new request to the queue
   m_queue.push_back(runnableRequest);
   
   // did we just transition from QUEUE_EMPTY to QUEUE_NOT_EMPTY?
   if (wasEmpty && !m_queue.empty()) {
      // signal QUEUE_NOT_EMPTY (wake up a worker thread)
      Logger::log(Logger::LogLevel::Debug, "signalling queue_not_empty");
      m_condQueueNotEmpty->notifyOne();
   }
   
   if (!m_mutex->unlock()) {
      Logger::error("unable to unlock mutex in addRequest");
      exit(1);
   }
   
   return true;
}

//******************************************************************************

std::shared_ptr<Runnable> ThreadPoolQueue::takeRequest() noexcept
{
   if (!m_isInitialized) {
      Logger::log(Logger::LogLevel::Warning, "ThreadPoolQueue::takeRequest queue not initialized");
      return nullptr;
   }

   // is the queue shut down?
   if (!m_isRunning) {
      return nullptr;
   }
   
   if (!m_mutex->haveValidMutex()) {
      Logger::error("don't have valid mutex in takeRequest");
      exit(1);
   }
   
   std::shared_ptr<Runnable> request = nullptr;
   
   if (m_isRunning) {
      
      if (!m_mutex->lock()) {
         Logger::error("unable to lock mutex in takeRequest");
         return nullptr;
      } else {
         // is the queue empty?
         if (m_queue.empty()) {
            Logger::log(Logger::LogLevel::Debug, "queue is empty, waiting for QUEUE_NOT_EMPTY event");
      
            // empty queue -- wait for QUEUE_NOT_EMPTY event
            if (!m_condQueueNotEmpty->wait(m_mutex)) {
               m_mutex->unlock();
               Logger::error("unable to wait on queue not empty condition");
               return nullptr;
            }
         }
   
         if (!m_queue.empty()) {
            // take a request from the queue
            request = m_queue.front();
            m_queue.pop_front();
         
            // did we empty the queue?
            if (m_queue.empty()) {
               Logger::log(Logger::LogLevel::Debug, "--- signalling queue_empty_event");
               // signal that queue is now empty
               m_condQueueEmpty->notifyOne();
            }
         }

         if (!m_mutex->unlock()) {
            Logger::error("unable to unlock mutex in takeRequest");
            exit(1);
         }
      }
   }
   
   return request;
}

//******************************************************************************

void ThreadPoolQueue::shutDown() noexcept
{
   if (m_isInitialized && m_isRunning) {
      MutexLock lock(*m_mutex);
      m_isRunning = false;
   
      if (!m_queue.empty()) {
         // wait for QUEUE_EMPTY event
         m_condQueueEmpty->wait(m_mutex);
      }
   }
}

//******************************************************************************

bool ThreadPoolQueue::isRunning() const noexcept
{
   return m_isRunning;
}

//******************************************************************************

bool ThreadPoolQueue::isEmpty() const noexcept
{
   MutexLock lock(*m_mutex);
   return(m_queue.empty());
}

//******************************************************************************

bool ThreadPoolQueue::isInitialized() const noexcept
{
   return m_isInitialized;
}

//******************************************************************************

