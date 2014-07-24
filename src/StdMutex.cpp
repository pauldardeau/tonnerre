// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#include "StdMutex.h"
#include "Logger.h"

static const std::string EMPTY_STRING = "";

//******************************************************************************

StdMutex::StdMutex() :
   StdMutex(EMPTY_STRING)
{
}

//******************************************************************************

StdMutex::StdMutex(const std::string& mutexName) :
   m_mutexName(mutexName),
   m_isLocked(false)
{
   Logger::logInstanceCreate("StdMutex");
}

//******************************************************************************

StdMutex::~StdMutex() noexcept
{
   Logger::logInstanceDestroy("StdMutex");

   if (m_isLocked) {
      unlock();
   }
}

//******************************************************************************

bool StdMutex::unlock() noexcept
{
   if (m_isLocked) {
      m_mutex.unlock();
      m_isLocked = false;
      return true;
   } else {
      return false;
   }
}

//******************************************************************************

bool StdMutex::lock() noexcept
{
   if (!m_isLocked) {
      m_mutex.lock();
      m_isLocked = true;
      return m_isLocked;
   } else {
      return false;
   }
}

//******************************************************************************

bool StdMutex::isLocked() const noexcept
{
   return m_isLocked;
}

//******************************************************************************

bool StdMutex::haveValidMutex() const noexcept
{
   return true;
}

//******************************************************************************

const std::string& StdMutex::getName() const noexcept
{
   return m_mutexName;
}

//******************************************************************************

