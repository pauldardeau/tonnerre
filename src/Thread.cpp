// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#include <cstdio>
#include <string>

#include "Thread.h"
#include "ThreadCompletionObserver.h"
#include "MutexLock.h"
#include "BasicException.h"
#include "Logger.h"

static const std::string EMPTY = "";

static const std::string ATTR_THREAD_ID     = "os_tid";
static const std::string ATTR_WORKER_ID     = "worker_id";

//******************************************************************************

Thread::Thread(Mutex& mutexAlive) noexcept :
   Thread(mutexAlive, nullptr)
{
}

//******************************************************************************

Thread::Thread(Mutex& mutexAlive, std::shared_ptr<Runnable> runnable) noexcept :
   m_runnable(runnable),
   m_isAlive(false),
   m_isPoolWorker(false),
   m_mutexAlive(mutexAlive)
{
   if (Logger::isLogging(Logger::LogLevel::Debug)) {
      Logger::debug("new thread created");
   }
}

//******************************************************************************

Thread::~Thread() noexcept
{
}

//******************************************************************************

void Thread::setPoolWorkerStatus(bool isPoolWorker) noexcept
{
   m_isPoolWorker = isPoolWorker;
}

//******************************************************************************

bool Thread::isPoolWorker() const noexcept
{
   return m_isPoolWorker;
}

//******************************************************************************

void Thread::run()
{
   // This method should never be called.  If you've subclassed Thread, then
   // you need to implement "void run()" in your derived class.
   throw BasicException("this method should not be called");
}

//******************************************************************************

bool Thread::isAlive() const noexcept
{
   bool rc;
   
   {
      Thread* pThis = const_cast<Thread*>(this);
      MutexLock lockMutexAlive(pThis->m_mutexAlive);
      rc = m_isAlive;
   }
   
   return rc;
}

//******************************************************************************

void Thread::setAlive(bool isAlive) noexcept
{
   MutexLock lockMutexAlive(m_mutexAlive);
   m_isAlive = isAlive;
}

//******************************************************************************

void Thread::registerThreadCompletionObserver(std::shared_ptr<ThreadCompletionObserver> observer) noexcept
{
   m_threadCompletionObserver = observer;
}

//******************************************************************************

void Thread::clearThreadCompletionObserver() noexcept
{
   m_threadCompletionObserver.reset();
}

//******************************************************************************

void Thread::notifyOnCompletion() noexcept
{
   if (m_threadCompletionObserver) {
      std::shared_ptr<Thread> thread =
         std::dynamic_pointer_cast<Thread>(shared_from_this());
      m_threadCompletionObserver->notifyThreadComplete(thread);
   }
}

//******************************************************************************

std::shared_ptr<Runnable> Thread::getRunnable() noexcept
{
   return m_runnable;
}

//******************************************************************************

void Thread::setAttribute(const std::string& key,
                                  const std::string& value) noexcept
{
   m_attributes.addPair(key, value);
}

//******************************************************************************

bool Thread::hasAttribute(const std::string& key) const noexcept
{
   return m_attributes.hasKey(key);
}

//******************************************************************************

const std::string& Thread::getAttribute(const std::string& key) const
{
   // may throw InvalidKeyException
   return m_attributes.getValue(key);
}

//******************************************************************************

void Thread::clearAttribute(const std::string& key) noexcept
{
   m_attributes.removePair(key);
}

//******************************************************************************

void Thread::setThreadId(const std::string& threadId) noexcept
{
   setAttribute(ATTR_THREAD_ID, threadId);
}

//******************************************************************************

bool Thread::hasThreadId() const noexcept
{
   return hasAttribute(ATTR_THREAD_ID);
}

//******************************************************************************

const std::string& Thread::getThreadId() const
{
   if (hasThreadId()) {
      return getAttribute(ATTR_THREAD_ID);
   } else {
      return EMPTY;
   }
}

//******************************************************************************

void Thread::setWorkerId(const std::string& workerId) noexcept
{
   setAttribute(ATTR_WORKER_ID, workerId);
}

//******************************************************************************

bool Thread::hasWorkerId() const noexcept
{
   return hasAttribute(ATTR_WORKER_ID);
}

//******************************************************************************

const std::string& Thread::getWorkerId() const
{
   if (hasWorkerId()) {
      return getAttribute(ATTR_WORKER_ID);
   } else {
      return EMPTY;
   }
}

//******************************************************************************

