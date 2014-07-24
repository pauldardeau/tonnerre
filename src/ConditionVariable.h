// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef __HttpServer__ConditionVariable__
#define __HttpServer__ConditionVariable__

#include <memory>

class Mutex;


/*!
 * ConditionVariable is an abstract base class for condition variables
 */
class ConditionVariable
{
public:
   virtual ~ConditionVariable() {}
   
   virtual bool wait(std::shared_ptr<Mutex> mutex) noexcept = 0;
   virtual void notifyOne() noexcept = 0;
   virtual void notifyAll() noexcept = 0;
   
};

#endif /* defined(__HttpServer__ConditionVariable__) */
