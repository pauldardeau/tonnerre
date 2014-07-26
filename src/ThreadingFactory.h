// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef THREADINGFACTORY_H
#define THREADINGFACTORY_H

#include <string>
#include <memory>


class Mutex;
class Thread;
class ConditionVariable;
class Runnable;
class ThreadPoolDispatcher;

/**
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
   
   /**
    * Retrieves the class singleton instance
    * @return the class singleton instance
    */
   static std::shared_ptr<ThreadingFactory> getThreadingFactory() noexcept;
   
   /**
    * Sets the specified ThreadingFactory instance to be the class singleton
    * @param threadingFactory the instance to use for the new class singleton
    */
   static void setThreadingFactory(std::shared_ptr<ThreadingFactory> threadingFactory) noexcept;

   /**
    * Constructs a ThreadingFactory instance based on the specified threading package
    * @param threadingPackage the package of threading primitives/library to use
    */
   explicit ThreadingFactory(ThreadingPackage threadingPackage) noexcept;
   
   /**
    * Destructor
    */
   ~ThreadingFactory() noexcept;
   
   // throws BasicException
   /**
    *
    * @param threadingPackage
    */
   void setMutexType(ThreadingPackage threadingPackage);
   
   /**
    * Create a new Mutex
    * @return pointer to newly created Mutex
    * @see Mutex()
    */
   std::shared_ptr<Mutex> createMutex();
   
   /**
    * Create a new named Mutex
    * @param mutexName the name for the new Mutex
    * @return pointer to the newly created Mutex
    * @see Mutex()
    */
   std::shared_ptr<Mutex> createMutex(const std::string& mutexName);
   
   /**
    * Create a new Thread
    * @return pointer to newly created Thread
    * @see Thread()
    */
   std::shared_ptr<Thread> createThread() noexcept;
   
   /**
    * Creates a new Thread to run the specified Runnable
    * @param runnable the Runnable object for the thread to run
    * @return pointer to the newly created Thread
    * @see Thread()
    * @see Runnable()
    */
   std::shared_ptr<Thread> createThread(std::shared_ptr<Runnable> runnable) noexcept;
   
   /**
    * Create a new ConditionVariable
    * @return pointer to the newly created ConditionVariable
    * @see ConditionVariable()
    */
   std::shared_ptr<ConditionVariable> createConditionVariable();
   
   /**
    * Creates a new ThreadPoolDispatcher
    * @param numberThreads the number of threads to initialize in the pool dispatcher
    * @return pointer to newly created ThreadPoolDispatcher
    */
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

#endif
