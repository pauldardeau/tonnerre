// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef HttpServer_ThreadPoolDispatcher_h
#define HttpServer_ThreadPoolDispatcher_h

#include <memory>


class Runnable;


/*!
 * ThreadPoolDispatcher is an abstract base class for handing off requests
 * to a thread pool to be dispatched.
 */
class ThreadPoolDispatcher
{
public:
   ThreadPoolDispatcher() noexcept {}
   virtual ~ThreadPoolDispatcher() noexcept {}
   
   virtual bool start() noexcept = 0;
   virtual bool stop() noexcept = 0;
   virtual bool addRequest(std::shared_ptr<Runnable> runnableRequest) noexcept = 0;
   

   // disallow copies
   ThreadPoolDispatcher(const ThreadPoolDispatcher&) = delete;
   ThreadPoolDispatcher(ThreadPoolDispatcher&&) = delete;
   ThreadPoolDispatcher& operator=(const ThreadPoolDispatcher&) = delete;
   ThreadPoolDispatcher& operator=(ThreadPoolDispatcher&&) = delete;

};

#endif
