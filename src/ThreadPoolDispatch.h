// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef __HttpServer__ThreadPoolDispatch__
#define __HttpServer__ThreadPoolDispatch__

#include "ThreadPoolDispatcher.h"

class Runnable;


/*!
 * ThreadPoolDispatch is a wrapper class for libDispatch (GCD)
 */
class ThreadPoolDispatch : public ThreadPoolDispatcher
{
public:
   ThreadPoolDispatch() noexcept;
   ~ThreadPoolDispatch() noexcept;
   
   // ThreadPoolDispatcher
   virtual bool start() noexcept override;
   virtual bool stop() noexcept override;
   virtual bool addRequest(std::shared_ptr<Runnable> runnableRequest) noexcept override;

   // disallow copies
   ThreadPoolDispatch(const ThreadPoolDispatch&) = delete;
   ThreadPoolDispatch(ThreadPoolDispatch&&) = delete;
   ThreadPoolDispatch& operator=(const ThreadPoolDispatch&) = delete;
   ThreadPoolDispatch& operator=(ThreadPoolDispatch&&) = delete;

   
private:
   bool m_isRunning;
   
};


#endif /* defined(__HttpServer__ThreadPoolDispatch__) */
