// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef __HttpServer__StdConditionVariable__
#define __HttpServer__StdConditionVariable__

#include <condition_variable>

#include "ConditionVariable.h"


/*!
 *
 */
class StdConditionVariable : public ConditionVariable
{
public:
   StdConditionVariable();
   ~StdConditionVariable();
   
   virtual bool wait(std::shared_ptr<Mutex> mutex) noexcept override;
   virtual void notifyOne() noexcept override;
   virtual void notifyAll() noexcept override;
   
   
   // disallow copies
   StdConditionVariable(const StdConditionVariable&);
   StdConditionVariable(StdConditionVariable&&);
   StdConditionVariable& operator=(const StdConditionVariable&);
   StdConditionVariable& operator=(StdConditionVariable&&);

private:
   std::condition_variable m_cond;
};

#endif /* defined(__HttpServer__StdConditionVariable__) */
