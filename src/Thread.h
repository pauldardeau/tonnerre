// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef __HttpServer__Thread__
#define __HttpServer__Thread__


#include "Runnable.h"
#include "KeyValuePairs.h"

class Mutex;
class ThreadCompletionObserver;


/*!
 * Thread is an abstract base class for working with threads.
 */
class Thread : public Runnable
{
public:
   explicit Thread(Mutex& mutexAlive) noexcept;
   Thread(Mutex& mutexAlive, std::shared_ptr<Runnable> runnable) noexcept;
   
   virtual ~Thread() noexcept;
   
   
   void setPoolWorkerStatus(bool isPoolWorker) noexcept;
   bool isPoolWorker() const noexcept;
   
   virtual bool start() noexcept = 0;
   virtual void run() override;
   
   bool isAlive() const noexcept;
   void setAlive(bool isAlive) noexcept;
   
   void registerThreadCompletionObserver(std::shared_ptr<ThreadCompletionObserver> observer) noexcept;
   void clearThreadCompletionObserver() noexcept;
   void notifyOnCompletion() noexcept;
   
   
   std::shared_ptr<Runnable> getRunnable() noexcept;
   
   
   void setAttribute(const std::string& key,
                     const std::string& value) noexcept;
   
   bool hasAttribute(const std::string& key) const noexcept;
   
   // throws InvalidKeyException
   const std::string& getAttribute(const std::string& key) const;
   
   void clearAttribute(const std::string& key) noexcept;
   
   void setThreadId(const std::string& threadId) noexcept;
   bool hasThreadId() const noexcept;
   const std::string& getThreadId() const;
   
   void setWorkerId(const std::string& workerId) noexcept;
   bool hasWorkerId() const noexcept;
   const std::string& getWorkerId() const;

   
   // disallow copying
   Thread(const Thread&) = delete;
   Thread(Thread&&) = delete;
   Thread& operator=(const Thread&) = delete;
   Thread& operator=(Thread&&) = delete;
   

   
private:
   std::shared_ptr<Runnable> m_runnable;
   bool m_isAlive;
   bool m_isPoolWorker;
   Mutex& m_mutexAlive;
   std::shared_ptr<ThreadCompletionObserver> m_threadCompletionObserver;
   KeyValuePairs m_attributes;
};


#endif /* defined(__HttpServer__Thread__) */
