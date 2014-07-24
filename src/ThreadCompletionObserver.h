// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef HttpServer_ThreadCompletionObserver_h
#define HttpServer_ThreadCompletionObserver_h

class Thread;


/*!
 *
 */
class ThreadCompletionObserver
{
public:
   virtual ~ThreadCompletionObserver() noexcept {}
   virtual void notifyThreadComplete(std::shared_ptr<Thread> thread) noexcept = 0;
};


#endif
