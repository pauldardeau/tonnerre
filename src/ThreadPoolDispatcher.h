// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef THREADPOOLDISPATCHER_H
#define THREADPOOLDISPATCHER_H

#include <memory>


class Runnable;


/**
 * ThreadPoolDispatcher is an abstract base class for handing off requests
 * to a thread pool to be dispatched.
 */
class ThreadPoolDispatcher
{
public:
   /**
    * Default constructor
    */
   ThreadPoolDispatcher() noexcept {}
   
   /**
    * Destructor
    */
   virtual ~ThreadPoolDispatcher() noexcept {}
   
   /**
    *
    * @return
    */
   virtual bool start() noexcept = 0;
   
   /**
    *
    * @return
    */
   virtual bool stop() noexcept = 0;
   
   /**
    *
    * @param runnableRequest
    * @return
    * @see Runnable()
    */
   virtual bool addRequest(std::shared_ptr<Runnable> runnableRequest) noexcept = 0;
   

   // disallow copies
   ThreadPoolDispatcher(const ThreadPoolDispatcher&) = delete;
   ThreadPoolDispatcher(ThreadPoolDispatcher&&) = delete;
   ThreadPoolDispatcher& operator=(const ThreadPoolDispatcher&) = delete;
   ThreadPoolDispatcher& operator=(ThreadPoolDispatcher&&) = delete;

};

#endif
