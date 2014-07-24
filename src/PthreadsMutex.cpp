// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#include <errno.h>

#include "PthreadsMutex.h"
#include "BasicException.h"
#include "Logger.h"

static const std::string EMPTY_STRING = "";


//******************************************************************************

PthreadsMutex::PthreadsMutex() :
   PthreadsMutex(EMPTY_STRING)
{
}

//******************************************************************************

PthreadsMutex::PthreadsMutex(const std::string& mutexName) :
   m_mutexName(mutexName),
   m_haveValidMutex(false),
   m_isLocked(false)
{
   Logger::logInstanceCreate("PthreadsMutex");
   char buffer[128];
   
   pthread_mutexattr_t attr;
   int rc = ::pthread_mutexattr_init(&attr);
   if (0 == rc) {
      rc = ::pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);

      if (0 == rc) {
         rc = ::pthread_mutex_init(&m_mutex, &attr);
         
         ::pthread_mutexattr_destroy(&attr);
      
         if (0 == rc) {
            m_haveValidMutex = true;
         } else {
            snprintf(buffer, 128, "unable to create pthreads mutex, rc=%d", rc);
            Logger::error(buffer);
            throw BasicException(buffer);
         }
      } else {
         snprintf(buffer, 128, "unable to set pthreads mutex type, rc=%d", rc);
         Logger::error(buffer);
         throw BasicException(buffer);
      }
   } else {
      snprintf(buffer, 128, "unable to initialize mutex attributes, rc=%d", rc);
      Logger::error(buffer);
      throw BasicException(buffer);
   }
}

//******************************************************************************

PthreadsMutex::~PthreadsMutex() noexcept
{
   Logger::logInstanceDestroy("PthreadsMutex");

   if (m_haveValidMutex) {
      unlock();
      ::pthread_mutex_destroy(&m_mutex);
   }
}

//******************************************************************************

bool PthreadsMutex::unlock() noexcept
{
   if (m_haveValidMutex) {
      const int rc = ::pthread_mutex_unlock(&m_mutex);
      if (0 == rc) {
         m_isLocked = false;
      } else {
         Logger::error("unable to unlock PthreadsMutex");
      }
      
      return !m_isLocked;
   } else {
      if (!m_haveValidMutex) {
         Logger::error("can't unlock PthreadsMutex, don't have valid mutex");
      }
      
      if (!m_isLocked) {
         Logger::error("can't unlock PthreadsMutex, not locked");
      }
      
      return false;
   }
}

//******************************************************************************

bool PthreadsMutex::lock() noexcept
{
   if (m_haveValidMutex) {
      
      const int rc = ::pthread_mutex_lock(&m_mutex);
      if (0 == rc) {
         m_isLocked = true;
      } else {
         std::string errorCode;
         
         switch(rc) {
            case EINVAL:
               errorCode = "EINVAL";
               break;
            case EAGAIN:
               errorCode = "EAGAIN";
               break;
            case EDEADLK:
               errorCode = "EDEADLK";
               break;
         }
         
         std::string buffer = "unable to lock mutex, pthread_mutex_lock rc=";
         buffer += errorCode;
         Logger::error(buffer);
      }
      
      return m_isLocked;
   } else {
      if (!m_haveValidMutex) {
         Logger::error("can't lock PthreadsMutex, don't have a valid Pthreads mutex");
      }
      
      if (m_isLocked) {
         Logger::error("can't lock PthreadsMutex, already locked");
      }
      
      return false;
   }
}

//******************************************************************************

bool PthreadsMutex::isLocked() const noexcept
{
   return m_haveValidMutex && m_isLocked;
}

//******************************************************************************

bool PthreadsMutex::haveValidMutex() const noexcept
{
   return m_haveValidMutex;
}

//******************************************************************************

const std::string& PthreadsMutex::getName() const noexcept
{
   return m_mutexName;
}

//******************************************************************************

