// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef PTHREADSCONDITIONVARIABLE_H
#define PTHREADSCONDITIONVARIABLE_H

#include <pthread.h>

#include "ConditionVariable.h"


/**
 *
 */
class PthreadsConditionVariable : public ConditionVariable
{
public:
   /**
    * Default constructor
    */
   PthreadsConditionVariable();
   
   /**
    * Destructor
    */
   ~PthreadsConditionVariable();
   
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
   PthreadsConditionVariable(const PthreadsConditionVariable&);
   PthreadsConditionVariable(PthreadsConditionVariable&&);
   PthreadsConditionVariable& operator=(const PthreadsConditionVariable&);
   PthreadsConditionVariable& operator=(PthreadsConditionVariable&&);
   
private:
   pthread_cond_t m_cond;
   bool m_initialized;
};


#endif

