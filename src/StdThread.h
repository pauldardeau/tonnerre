// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef STDTHREAD_H
#define STDTHREAD_H

#include <thread>

#include "Thread.h"
#include "StdMutex.h"


/**
 * StdThread is a Thread that's implemented using std::thread (C++ 11).
 */
class StdThread : public Thread
{
public:
   /**
    * Default constructor
    */
   StdThread() noexcept;
   
   /**
    *
    * @param runnable
    * @see Runnable()
    */
   explicit StdThread(std::shared_ptr<Runnable> runnable) noexcept;
   
   /**
    * Destructor
    */
   virtual ~StdThread() noexcept;
   
   /**
    *
    * @return
    */
   virtual bool start() noexcept override;
   
   /**
    *
    */
   virtual void run() override;
   
   /**
    *
    * @param thread
    */
   void runThread(StdThread* thread) noexcept;
   
   /**
    *
    * @return
    */
   std::thread::native_handle_type getHandle() noexcept;
   
   /**
    *
    * @return
    */
   std::thread::id getStdThreadId() const noexcept;
   
   // disallow copying
   StdThread(const StdThread&) = delete;
   StdThread(StdThread&&) = delete;
   StdThread& operator=(const StdThread&) = delete;
   StdThread& operator=(StdThread&&) = delete;
   
   
private:
   std::thread m_thread;
   StdMutex m_mutexAlive;
   
};

#endif
