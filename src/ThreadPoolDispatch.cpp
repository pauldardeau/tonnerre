// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef __linux__
#include <dispatch/dispatch.h>
#include <dispatch/queue.h>
#endif

#include "ThreadPoolDispatch.h"
#include "Runnable.h"
#include "Logger.h"
#include "BasicException.h"

//******************************************************************************

ThreadPoolDispatch::ThreadPoolDispatch() noexcept :
   m_isRunning(false)
{
   Logger::logInstanceCreate("ThreadPoolDispatch");
}

//******************************************************************************

ThreadPoolDispatch::~ThreadPoolDispatch() noexcept
{
   Logger::logInstanceDestroy("ThreadPoolDispatch");
}

//******************************************************************************

bool ThreadPoolDispatch::start() noexcept
{
   m_isRunning = true;
   return true;
}

//******************************************************************************

bool ThreadPoolDispatch::stop() noexcept
{
   m_isRunning = false;
   return true;
}

//******************************************************************************

bool ThreadPoolDispatch::addRequest(std::shared_ptr<Runnable> runnableRequest) noexcept
{
   if (!m_isRunning || !runnableRequest) {
      return false;
   }
   
#ifndef __linux__
   dispatch_queue_t queue =
      dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
   
   dispatch_async(queue, ^{
      try {
         runnableRequest->run();
      }
      catch (const BasicException& be)
      {
         Logger::error("exception running request: " + be.whatString());
      }
      catch (const std::exception& e)
      {
         Logger::error("exception running request: " + std::string(e.what()));
      }
      catch (...)
      {
         Logger::error("unknown exception running request");
      }
      
      runnableRequest->notifyOnCompletion();
      
   });
   
   return true;
#else
   return false;
#endif
}

//******************************************************************************

