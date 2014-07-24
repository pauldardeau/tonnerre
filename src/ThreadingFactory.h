// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef __HttpServer__ThreadingFactory__
#define __HttpServer__ThreadingFactory__

#include <string>
#include <memory>


class Mutex;
class Thread;
class ConditionVariable;
class Runnable;
class ThreadPoolDispatcher;

/*!
 * ThreadingFactory is a factory for creating Thread, Mutex, and ThreadPoolDispatcher
 * instances based on the desired ThreadingPackage.
 */
class ThreadingFactory : public std::enable_shared_from_this<ThreadingFactory>
{
public:
   enum class ThreadingPackage {
      CPP_11,
      PTHREADS,
      GCD_LIBDISPATCH
   };
   
   static std::shared_ptr<ThreadingFactory> getThreadingFactory() noexcept;
   static void setThreadingFactory(std::shared_ptr<ThreadingFactory> threadingFactory) noexcept;

   explicit ThreadingFactory(ThreadingPackage threadingPackage) noexcept;
   ~ThreadingFactory() noexcept;
   
   // throws BasicException
   void setMutexType(ThreadingPackage threadingPackage);
   
   std::shared_ptr<Mutex> createMutex();
   std::shared_ptr<Mutex> createMutex(const std::string& mutexName);
   
   std::shared_ptr<Thread> createThread() noexcept;
   std::shared_ptr<Thread> createThread(std::shared_ptr<Runnable> runnable) noexcept;
   
   std::shared_ptr<ConditionVariable> createConditionVariable();
   
   std::shared_ptr<ThreadPoolDispatcher> createThreadPoolDispatcher(int numberThreads) noexcept;
   
   // disallow copies
   ThreadingFactory(const ThreadingFactory&) = delete;
   ThreadingFactory(ThreadingFactory&&) = delete;
   ThreadingFactory& operator=(const ThreadingFactory&) = delete;
   ThreadingFactory& operator=(ThreadingFactory&&) = delete;

private:
   ThreadingPackage m_threadingPackage;
   ThreadingPackage m_packageMutexType;

   static std::shared_ptr<ThreadingFactory> threadingFactoryInstance;

};

#endif /* defined(__HttpServer__ThreadingFactory__) */
