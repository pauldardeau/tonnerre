// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#include <sstream>

#include "StdThread.h"
#include "BasicException.h"
#include "Logger.h"


//******************************************************************************
//******************************************************************************

void StdThread::runThread(StdThread* thread) noexcept
{
   std::stringstream ss;
   ss << std::this_thread::get_id();
   const std::string threadIdString = ss.str();
   
   thread->setThreadId(threadIdString);
   
   //unsigned long rc = 0L;
   
   try
   {
      thread->setAlive(true);
      
      // if we had a runnable passed in on the constructor, use it.  otherwise,
      // call "run" on the thread object itself.
      
      std::shared_ptr<Runnable> runnable = thread->getRunnable();
      
      if (runnable) {
         runnable->run();
      } else {
         thread->run();
      }
   }
   catch (...)
   {
      //rc = 1L;
      Logger::error("StdThread::runThread exception caught running thread");
   }
   
   thread->setAlive(false);
   thread->notifyOnCompletion();
}

//******************************************************************************
//******************************************************************************

StdThread::StdThread() noexcept :
   StdThread(nullptr)
{
}

//******************************************************************************

StdThread::StdThread(std::shared_ptr<Runnable> runnable) noexcept :
   Thread(m_mutexAlive, runnable)
{
   Logger::logInstanceCreate("StdThread");
}

//******************************************************************************

StdThread::~StdThread() noexcept
{
   Logger::logInstanceDestroy("StdThread");
}

//******************************************************************************

bool StdThread::start() noexcept
{
   bool isSuccess = false;
   
   //TODO: research and fix!
   // when the following line is uncommented, the following compiler error
   // is produced:
   // "Attempt to use a deleted function".
   //m_thread = std::thread(&StdThread::runThread, this);
   
   return isSuccess;
}

//******************************************************************************

void StdThread::run()
{
   // This method should never be called.  If you've subclassed Thread, then
   // you need to implement "void run()" in your derived class.
   throw BasicException("this method should not be called");
}

//******************************************************************************

std::thread::native_handle_type StdThread::getHandle() noexcept
{
   return m_thread.native_handle();
}

//******************************************************************************

std::thread::id StdThread::getStdThreadId() const noexcept
{
   return m_thread.get_id();
}

//******************************************************************************

