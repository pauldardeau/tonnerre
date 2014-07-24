// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#include <cstdio>
#include <string>

#include "PthreadsThread.h"
#include "MutexLock.h"
#include "BasicException.h"
#include "Logger.h"


//******************************************************************************

PthreadsThread::PthreadsThread() noexcept :
   PthreadsThread(nullptr)
{
}

//******************************************************************************

PthreadsThread::PthreadsThread(std::shared_ptr<Runnable> runnable) noexcept :
   Thread(m_mutexAlive, runnable),
   m_threadHandle(0),
   m_exitCode(1L)
{
   Logger::logInstanceCreate("PthreadsThread");
}

//******************************************************************************

PthreadsThread::~PthreadsThread() noexcept
{
   Logger::logInstanceDestroy("PthreadsThread");
}

//******************************************************************************

void* PthreadsThread::runThread(void* pArgs) noexcept
{
   PthreadsThread* pThread = (PthreadsThread*) pArgs;

   if (!pThread) {
      return (void*) 1L;
   }

   char threadId[128];
   std::snprintf(threadId, 128, "%lx", (long unsigned int) pThread);
   pThread->setThreadId(threadId);
   
   unsigned long rc = 0L;

   try
   {
      pThread->setAlive(true);

      // if we had a runnable passed in on the constructor, use it.  otherwise,
      // call "run" on the thread object itself.
      
      std::shared_ptr<Runnable> runnable = pThread->getRunnable();

      if (runnable) {
         runnable->run();
      } else {
         pThread->run();
      }
   }
   catch (...)
   {
      rc = 1L;
      Logger::error("Thread::runThread exception caught running thread");
   }

   pThread->setAlive(false);
   pThread->notifyOnCompletion();
   
   return (void*) rc;
}

//******************************************************************************

bool PthreadsThread::start() noexcept
{
   bool isSuccess = false;

   if (0 == ::pthread_create(&m_threadHandle, 0, runThread, (void*) this)) {
      isSuccess = true;
   }

   return isSuccess;
}

//******************************************************************************

void PthreadsThread::run()
{
   // This method should never be called.  If you've subclassed Thread, then
   // you need to implement "void run()" in your derived class.
   throw BasicException("this method should not be called");
}

//******************************************************************************

unsigned long PthreadsThread::getExitCode() const noexcept
{
   unsigned long rc;

   {
      PthreadsThread* pThis = const_cast<PthreadsThread*>(this);
      MutexLock lockMutexExitCode(pThis->m_mutexExitCode);
      rc = m_exitCode;
   }

   return rc;
}

//******************************************************************************

pthread_t PthreadsThread::getHandle() noexcept
{
   return m_threadHandle;
}

//******************************************************************************

