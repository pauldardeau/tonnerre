// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef C10KServer_MutexLock_h
#define C10KServer_MutexLock_h

#include "Mutex.h"


/*!
 * MutexLock is a convenience class for locking and unlocking a Mutex. This
 * class is meant to be used with RAII (i.e., on the stack) to get a lock
 * and then have the destructor release the lock.
 */
class MutexLock
{
public:
   explicit MutexLock(Mutex& mutex) noexcept :
      m_mutex(mutex),
      m_isLocked(false)
   {
      if (m_mutex.haveValidMutex()) {
         m_isLocked = m_mutex.lock();
      }
   }
    
   ~MutexLock() noexcept
   {
      unlock();
   }
    
   void unlock() noexcept
   {
      if (m_isLocked) {
         if (m_mutex.unlock()) {
            m_isLocked = false;
         }
      }
   }

   MutexLock() = delete;
   MutexLock(const Mutex&) = delete;
   MutexLock(Mutex&&) = delete;
   Mutex& operator=(const Mutex&) = delete;
   Mutex& operator=(Mutex&&) = delete;
   
    
private:
   Mutex& m_mutex;
   bool m_isLocked;
};


#endif
