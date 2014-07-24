// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#include "ThreadingFactory.h"
#include "PthreadsMutex.h"
#include "StdMutex.h"
#include "PthreadsThread.h"
#include "StdThread.h"
#include "ThreadPool.h"
#include "ThreadPoolDispatch.h"
#include "BasicException.h"
#include "Logger.h"
#include "PthreadsConditionVariable.h"
#include "StdConditionVariable.h"


std::shared_ptr<ThreadingFactory> ThreadingFactory::threadingFactoryInstance = nullptr;

//******************************************************************************

void ThreadingFactory::setThreadingFactory(std::shared_ptr<ThreadingFactory> threadingFactory) noexcept
{
   threadingFactoryInstance = threadingFactory;
}

//******************************************************************************

std::shared_ptr<ThreadingFactory> ThreadingFactory::getThreadingFactory() noexcept
{
   return threadingFactoryInstance;
}

//******************************************************************************

ThreadingFactory::ThreadingFactory(ThreadingPackage threadingPackage) noexcept :
   m_threadingPackage(threadingPackage),
   m_packageMutexType(threadingPackage)
{
   Logger::logInstanceCreate("ThreadingFactory");

}

//******************************************************************************

ThreadingFactory::~ThreadingFactory() noexcept
{
   Logger::logInstanceDestroy("ThreadingFactory");
}

//******************************************************************************

void ThreadingFactory::setMutexType(ThreadingPackage threadingPackage)
{
   if (m_threadingPackage == ThreadingPackage::GCD_LIBDISPATCH) {
      if (threadingPackage != ThreadingPackage::GCD_LIBDISPATCH) {
         m_packageMutexType = threadingPackage;
      } else {
         throw BasicException("Invalid mutex type. must be PTHREADS or CPP_11");
      }
   } else {
      // ignore it
      Logger::warning("Ignoring setMutexType for non GCD_LIBDISPATCH");
   }
}

//******************************************************************************

std::shared_ptr<Mutex> ThreadingFactory::createMutex()
{
   if (m_packageMutexType == ThreadingPackage::CPP_11) {
      return std::shared_ptr<Mutex>(new StdMutex());
   } else if (m_packageMutexType == ThreadingPackage::PTHREADS) {
      return std::shared_ptr<Mutex>(new PthreadsMutex());
   } else if (m_packageMutexType == ThreadingPackage::GCD_LIBDISPATCH) {
      Logger::error("invalid package type for mutexes. see setMutexType");
      throw BasicException("Invalid threading package for mutexes");
   }
   
   return nullptr;
}

//******************************************************************************

std::shared_ptr<Mutex> ThreadingFactory::createMutex(const std::string& mutexName)
{
   if (m_packageMutexType == ThreadingPackage::CPP_11) {
      return std::shared_ptr<Mutex>(new StdMutex(mutexName));
   } else if (m_packageMutexType == ThreadingPackage::PTHREADS) {
      return std::shared_ptr<Mutex>(new PthreadsMutex(mutexName));
   } else if (m_packageMutexType == ThreadingPackage::GCD_LIBDISPATCH) {
      Logger::error("invalid package type for mutexes. see setMutexType");
      throw BasicException("Invalid threading package for mutexes");
   }
   
   return nullptr;
}

//******************************************************************************

std::shared_ptr<Thread> ThreadingFactory::createThread() noexcept
{
   return createThread(nullptr);
}

//******************************************************************************

std::shared_ptr<Thread> ThreadingFactory::createThread(std::shared_ptr<Runnable> runnable) noexcept
{
   if (m_threadingPackage == ThreadingPackage::CPP_11) {
      if (runnable != nullptr) {
         return std::shared_ptr<Thread>(new StdThread(runnable));
      } else {
         return std::shared_ptr<Thread>(new StdThread());
      }
   } else if (m_threadingPackage == ThreadingPackage::PTHREADS) {
      if (runnable != nullptr) {
         return std::shared_ptr<Thread>(new PthreadsThread(runnable));
      } else {
         return std::shared_ptr<Thread>(new PthreadsThread());
      }
   } else if (m_threadingPackage == ThreadingPackage::GCD_LIBDISPATCH) {
      Logger::error("createThread should not be called on GCD_LIBDISPATCH");
   }
   
   return nullptr;
}

//******************************************************************************

std::shared_ptr<ConditionVariable> ThreadingFactory::createConditionVariable()
{
   if (m_packageMutexType == ThreadingPackage::CPP_11) {
      return std::shared_ptr<ConditionVariable>(new StdConditionVariable());
   } else if (m_packageMutexType == ThreadingPackage::PTHREADS) {
      return std::shared_ptr<ConditionVariable>(new PthreadsConditionVariable());
   } else if (m_packageMutexType == ThreadingPackage::GCD_LIBDISPATCH) {
      Logger::error("createConditionVariable should not be called on GCD_LIBDISPATCH");
      throw BasicException("Invalid threading package for condition variables");
   }
   
   return nullptr;
}

//******************************************************************************

std::shared_ptr<ThreadPoolDispatcher> ThreadingFactory::createThreadPoolDispatcher(int numberThreads) noexcept
{
   if (m_threadingPackage == ThreadingPackage::GCD_LIBDISPATCH) {
      return std::shared_ptr<ThreadPoolDispatcher>(new ThreadPoolDispatch());  // don't specify number threads
   } else {
      return std::shared_ptr<ThreadPoolDispatcher>(new ThreadPool(shared_from_this(), numberThreads));
   }
   
   return nullptr;
}

//******************************************************************************

