// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef __HttpServer__StdMutex__
#define __HttpServer__StdMutex__

#include <string>
#include <mutex>

#include "Mutex.h"


/*!
 * StdMutex is a Mutex that is implemented as a wrapper around std::mutex (C++ 11).
 */
class StdMutex : public Mutex
{
public:
   StdMutex();
   explicit StdMutex(const std::string& mutexName);
   ~StdMutex() noexcept;
   
   // Mutex
   virtual bool unlock() noexcept override;
   virtual bool lock() noexcept override;
   virtual bool isLocked() const noexcept override;
   virtual bool haveValidMutex() const noexcept override;
   
   std::mutex& getPlatformPrimitive() noexcept
   {
      return m_mutex;
   }
   
   
   const std::string& getName() const noexcept;
   
   
   // copying not allowed
   StdMutex(const StdMutex&) = delete;
   StdMutex(StdMutex&&) = delete;
   StdMutex& operator=(const StdMutex&) = delete;
   StdMutex& operator=(StdMutex&&) = delete;
   
private:
   std::mutex  m_mutex;
   std::string m_mutexName;
   bool m_isLocked;
   
};

#endif /* defined(__HttpServer__StdMutex__) */
