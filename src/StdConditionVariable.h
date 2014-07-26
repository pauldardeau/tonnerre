// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef STDCONDITIONVARIABLE_H
#define STDCONDITIONVARIABLE_H

#include <condition_variable>

#include "ConditionVariable.h"


/**
 *
 */
class StdConditionVariable : public ConditionVariable
{
public:
   /**
    * Default constructor
    */
   StdConditionVariable();
   
   /**
    * Destructor
    */
   ~StdConditionVariable();
   
   /**
    *
    * @param mutex
    * @return
    * @see Mutex()
    */
   virtual bool wait(std::shared_ptr<Mutex> mutex) noexcept override;
   
   /**
    *
    */
   virtual void notifyOne() noexcept override;
   
   /**
    *
    */
   virtual void notifyAll() noexcept override;
   
   
   // disallow copies
   StdConditionVariable(const StdConditionVariable&);
   StdConditionVariable(StdConditionVariable&&);
   StdConditionVariable& operator=(const StdConditionVariable&);
   StdConditionVariable& operator=(StdConditionVariable&&);

private:
   std::condition_variable m_cond;
};

#endif
