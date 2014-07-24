// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#include "StdConditionVariable.h"
#include "StdMutex.h"
#include "BasicException.h"
#include "Logger.h"

//******************************************************************************

StdConditionVariable::StdConditionVariable()
{
   Logger::logInstanceCreate("StdConditionVariable");
}

//******************************************************************************

StdConditionVariable::~StdConditionVariable()
{
   Logger::logInstanceDestroy("StdConditionVariable");
}

//******************************************************************************

bool StdConditionVariable::wait(std::shared_ptr<Mutex> mutex) noexcept
{
   if (mutex) {
      std::shared_ptr<StdMutex> stdMutex =
         std::dynamic_pointer_cast<StdMutex>(mutex);
      
      if (stdMutex) {
         std::unique_lock<std::mutex> lock(stdMutex->getPlatformPrimitive());
         m_cond.wait(lock);
         return true;
      } else {
         Logger::error("mutex must be an instance of StdMutex");
      }
   } else {
      Logger::error("no mutex given to wait on");
   }
   return false;
}

//******************************************************************************

void StdConditionVariable::notifyOne() noexcept
{
   m_cond.notify_one();
}

//******************************************************************************

void StdConditionVariable::notifyAll() noexcept
{
   m_cond.notify_all();
}

//******************************************************************************

