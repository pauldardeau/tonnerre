// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef __HttpServer__PthreadsConditionVariable__
#define __HttpServer__PthreadsConditionVariable__

#include <pthread.h>

#include "ConditionVariable.h"


/*!
 *
 */
class PthreadsConditionVariable : public ConditionVariable
{
public:
   PthreadsConditionVariable();
   ~PthreadsConditionVariable();
   
   virtual bool wait(std::shared_ptr<Mutex> mutex) noexcept override;
   virtual void notifyOne() noexcept override;
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


#endif /* defined(__HttpServer__PthreadsConditionVariable__) */

