// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef HttpServer_ThreadInfo_h
#define HttpServer_ThreadInfo_h

#include <string>

/*!
 * ThreadInfo is used to track state of a Thread within a ThreadPool.
 */
class ThreadInfo
{
public:
   std::string threadId;
   bool isBusy;
   
   
   explicit ThreadInfo(const std::string& theThreadId) noexcept :
      threadId(theThreadId),
      isBusy(false)
   {
   }
   
   ThreadInfo(const ThreadInfo& copy) noexcept :
      threadId(copy.threadId),
      isBusy(copy.isBusy)
   {
   }
   
   ThreadInfo(ThreadInfo&& move) noexcept :
      threadId(std::move(move.threadId)),
      isBusy(move.isBusy)
   {
   }
   
   ~ThreadInfo() noexcept
   {
   }
   
   ThreadInfo& operator=(const ThreadInfo& copy) noexcept
   {
      if (this == &copy) {
         return *this;
      }
      
      threadId = copy.threadId;
      isBusy = copy.isBusy;
      
      return *this;
   }
   
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
