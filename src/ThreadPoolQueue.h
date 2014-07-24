// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef HttpServer_ThreadPoolQueue_h
#define HttpServer_ThreadPoolQueue_h

#include <deque>
#include <memory>

class ConditionVariable;
class Mutex;
class Runnable;
class ThreadingFactory;


/*!
 * ThreadPoolQueue is an abstract base class for a queue being serviced
 * by a thread from a thread pool.
 */
class ThreadPoolQueue
{
public:
   explicit ThreadPoolQueue(std::shared_ptr<ThreadingFactory> threadingFactory) noexcept;
   virtual ~ThreadPoolQueue() noexcept;
   
   virtual bool addRequest(std::shared_ptr<Runnable> runnableRequest) noexcept;
   virtual std::shared_ptr<Runnable> takeRequest() noexcept;
   virtual void shutDown() noexcept;
   virtual bool isRunning() const noexcept;
   virtual bool isEmpty() const noexcept;
   virtual bool isInitialized() const noexcept;
   
   // disallow copies
   ThreadPoolQueue(const ThreadPoolQueue&) = delete;
   ThreadPoolQueue(ThreadPoolQueue&&) = delete;
   ThreadPoolQueue& operator=(const ThreadPoolQueue&) = delete;
   ThreadPoolQueue& operator=(ThreadPoolQueue&&) = delete;
   
private:
   std::shared_ptr<ThreadingFactory> m_threadingFactory;
   std::deque<std::shared_ptr<Runnable>> m_queue;
   std::shared_ptr<Mutex> m_mutex;
   std::shared_ptr<ConditionVariable> m_condQueueEmpty;
   std::shared_ptr<ConditionVariable> m_condQueueNotEmpty;
   bool m_isInitialized;
   bool m_isRunning;
};


#endif
