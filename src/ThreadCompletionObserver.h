// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef THREADCOMPLETIONOBSERVER_H
#define THREADCOMPLETIONOBSERVER_H

class Thread;


/**
 *
 */
class ThreadCompletionObserver
{
public:
   /**
    * Destructor
    */
   virtual ~ThreadCompletionObserver() noexcept {}
   
   /**
    *
    * @param thread
    * @see Thread()
    */
   virtual void notifyThreadComplete(std::shared_ptr<Thread> thread) noexcept = 0;
};


#endif
