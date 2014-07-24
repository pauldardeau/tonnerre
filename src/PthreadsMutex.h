// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef C10KServer_Mutex_h
#define C10KServer_Mutex_h

#include <string>
#include <pthread.h>

#include "Mutex.h"

/*!
 * PthreadsMutex is a wrapper class for working with mutexes from pthreads.
 */
class PthreadsMutex : public Mutex
{
public:
   PthreadsMutex();
   explicit PthreadsMutex(const std::string& mutexName);
   ~PthreadsMutex() noexcept;

   // Mutex
   virtual bool unlock() noexcept override;
   virtual bool lock() noexcept override;
   virtual bool isLocked() const noexcept override;
   virtual bool haveValidMutex() const noexcept override;
   
   pthread_mutex_t& getPlatformPrimitive() noexcept
   {
      return m_mutex;
   }
   
   
   const std::string& getName() const noexcept;
   

   // copying not allowed
   PthreadsMutex(const PthreadsMutex&) = delete;
   PthreadsMutex(PthreadsMutex&&) = delete;
   PthreadsMutex& operator=(const PthreadsMutex&) = delete;
   PthreadsMutex& operator=(PthreadsMutex&&) = delete;
    
private:
   pthread_mutex_t  m_mutex;
   std::string m_mutexName;
   bool m_haveValidMutex;
   bool m_isLocked;
    
};


#endif
