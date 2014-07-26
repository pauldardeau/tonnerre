// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef RUNNABLE_H
#define RUNNABLE_H

#include <string>
#include <memory>

#include "RunCompletionObserver.h"
#include "Logger.h"


/**
 * Runnable is an abstract base class that's conceptually very similar to
 * Java's Runnable interface for classes that can be used in background
 * threads.
 */
class Runnable : public virtual std::enable_shared_from_this<Runnable>
{
private:
   std::shared_ptr<RunCompletionObserver> m_completionObserver;
   std::string m_runByThreadWorkerId;
   int m_runByThreadId;
   
public:
   /**
    * Default constructor
    */
   Runnable() noexcept :
      m_runByThreadId(0)
   {
   }
   
   
   // disallow copies
   Runnable(const Runnable&) noexcept = delete;
   Runnable(Runnable&&) noexcept = delete;
   Runnable& operator=(const Runnable&) noexcept = delete;
   Runnable& operator=(Runnable&&) noexcept = delete;
   

   /**
    * Destructor
    */
   virtual ~Runnable() noexcept {}
    

   /**
    *
    */
   virtual void run() = 0;
   
   /**
    *
    * @param runByThreadId
    */
   virtual void setRunByThreadId(int runByThreadId) noexcept
   {
      m_runByThreadId = runByThreadId;
   }
   
   /**
    *
    * @return
    */
   virtual int getRunByThreadId() const noexcept
   {
      return m_runByThreadId;
   }
   
   /**
    *
    * @param runByThreadWorkerId
    */
   virtual void setRunByThreadWorkerId(const std::string& runByThreadWorkerId) noexcept
   {
      m_runByThreadWorkerId = runByThreadWorkerId;
   }
   
   /**
    *
    * @return
    */
   virtual const std::string& getRunByThreadWorkerId() const noexcept
   {
      return m_runByThreadWorkerId;
   }
   
   /**
    *
    * @param completionObserver
    * @see RunCompletionObserver()
    */
   void setCompletionObserver(std::shared_ptr<RunCompletionObserver> completionObserver) noexcept
   {
       m_completionObserver = completionObserver;
   }
   
   /**
    * This should only be called AFTER the run method has completed
    */
   virtual void notifyOnCompletion() noexcept
   {
      if (m_completionObserver) {
         m_completionObserver->notifyRunComplete(shared_from_this());
      }
   }
};


#endif
