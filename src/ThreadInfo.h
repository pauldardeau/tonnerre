// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef THREADINFO_H
#define THREADINFO_H

#include <string>

/**
 * ThreadInfo is used to track state of a Thread within a ThreadPool.
 */
class ThreadInfo
{
public:
   std::string threadId;
   bool isBusy;
   
   /**
    * Constructs a new ThreadInfo with the specified thread id
    * @param theThreadId the id for the thread
    */
   explicit ThreadInfo(const std::string& theThreadId) noexcept :
      threadId(theThreadId),
      isBusy(false)
   {
   }
   
   /**
    * Copy constructor
    * @param copy the source of the copy
    */
   ThreadInfo(const ThreadInfo& copy) noexcept :
      threadId(copy.threadId),
      isBusy(copy.isBusy)
   {
   }
   
   /**
    * Move constructor
    * @param move the source of the move
    */
   ThreadInfo(ThreadInfo&& move) noexcept :
      threadId(std::move(move.threadId)),
      isBusy(move.isBusy)
   {
   }
   
   /**
    * Destructor
    */
   ~ThreadInfo() noexcept
   {
   }
   
   /**
    * Copy operator
    * @param copy the source of the copy
    * @return reference to the target of the copy
    */
   ThreadInfo& operator=(const ThreadInfo& copy) noexcept
   {
      if (this == &copy) {
         return *this;
      }
      
      threadId = copy.threadId;
      isBusy = copy.isBusy;
      
      return *this;
   }
   
   /**
    * Move operator
    * @param move the source of the move
    * @return reference to the target of the move
    */
   ThreadInfo& operator=(ThreadInfo&& move) noexcept
   {
      if (this == &move) {
         return *this;
      }
      
      threadId = std::move(move.threadId);
      isBusy = move.isBusy;
      
      return *this;
   }
   
};


#endif
