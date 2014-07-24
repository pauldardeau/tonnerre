// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef HttpServer_RunCompletionObserver_h
#define HttpServer_RunCompletionObserver_h


class Runnable;

/**
 * RunCompletionObserver is an interface used for callbacks to be notified
 * when a Runnable has completed its run.
 */
class RunCompletionObserver
{
public:
   /**
    * Destructor
    */
   virtual ~RunCompletionObserver() noexcept {}
   
   /**
    * Notifies the observer that the Runnable has completed
    * @param runnable the Runnable that has completed
    */
   virtual void notifyRunComplete(std::shared_ptr<Runnable> runnable) noexcept = 0;
};


#endif
