// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef __HttpServer__StdThread__
#define __HttpServer__StdThread__

#include <thread>

#include "Thread.h"
#include "StdMutex.h"


/*!
 * StdThread is a Thread that's implemented using std::thread (C++ 11).
 */
class StdThread : public Thread
{
public:
   StdThread() noexcept;
   explicit StdThread(std::shared_ptr<Runnable> runnable) noexcept;
   virtual ~StdThread() noexcept;
   
   virtual bool start() noexcept override;
   virtual void run() override;
   
   void runThread(StdThread* thread) noexcept;
   
   std::thread::native_handle_type getHandle() noexcept;
   
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

#endif /* defined(__HttpServer__StdThread__) */
