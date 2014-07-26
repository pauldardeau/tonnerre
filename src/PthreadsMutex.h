// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef PTHREADSMUTEX_H
#define PTHREADSMUTEX_H

#include <string>
#include <pthread.h>

#include "Mutex.h"

/**
 * PthreadsMutex is a wrapper class for working with mutexes from pthreads.
 */
class PthreadsMutex : public Mutex
{
public:
   /**
    * Default constructor
    */
   PthreadsMutex();
   
   /**
    * Constructs mutex with a name
    * @param mutexName the name of the mutex
    */
   explicit PthreadsMutex(const std::string& mutexName);
   
   /**
    * Destructor
    */
   ~PthreadsMutex() noexcept;

   // Mutex
   /**
    *
    * @return
    */
   virtual bool unlock() noexcept override;
   
   /**
    *
    * @return
    */
   virtual bool lock() noexcept override;
   
   /**
    *
    * @return
    */
   virtual bool isLocked() const noexcept override;
   
   /**
    *
    * @return
    */
   virtual bool haveValidMutex() const noexcept override;
   
   /**
    * Retrieves the primitive data type for the underlying platform
    * @return the platform's primitive data type for the mutex
    */
   pthread_mutex_t& getPlatformPrimitive() noexcept
   {
      return m_mutex;
   }
   
   /**
    * Retrieves the name of the mutex
    * @return name of the mutex
    */
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
