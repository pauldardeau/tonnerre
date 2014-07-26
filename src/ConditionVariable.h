// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef CONDITIONVARIABLE_H
#define CONDITIONVARIABLE_H

#include <memory>

class Mutex;


/**
 * ConditionVariable is an interface (abstract base class) for condition variables
 */
class ConditionVariable
{
public:
   virtual ~ConditionVariable() {}
   
   /**
    * Wait for the condition to occur
    * @param mutex the mutex lock that the caller currently has locked
    * @return boolean
    * @see Mutex()
    */
   virtual bool wait(std::shared_ptr<Mutex> mutex) noexcept = 0;
   
   /**
    * Notify (wake up) a single waiting thread that the condition has occurred
    */
   virtual void notifyOne() noexcept = 0;
   
   /**
    * Notify (wake up) all threads waiting that the condition has occurred
    */
   virtual void notifyAll() noexcept = 0;
   
};

#endif
