// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef KQUEUESERVER_H
#define KQUEUESERVER_H

#include "KernelEventServer.h"

class SocketServiceHandler;
class Mutex;


//#if defined(__APPLE__) || defined(__FreeBSD__)
#ifndef __linux__
#define KQUEUE_SUPPORT 1
#include <sys/event.h>
#endif


/**
 * KqueueServer is a wrapper for working with the kqueue API. The kqueue API
 * is a high-performance kernel event mechanism on FreeBSD and OSX.
 */
class KqueueServer : public KernelEventServer
{
public:
   /**
    * Determines if the current platform supports the kqueue mechanism
    * @return boolean indicating if kqueue mechanism is supported
    */
   static bool isSupportedPlatform() noexcept;
   
   /**
    *
    * @param fdMutex
    * @param hwmConnectionsMutex
    */
   KqueueServer(Mutex& fdMutex, Mutex& hwmConnectionsMutex) noexcept;
   
   /**
    * Destructor
    */
   ~KqueueServer() noexcept;
   
   // KernelEventServer
   /**
    *
    * @param socketServiceHandler
    * @param serverPort
    * @param maxConnections
    * @return
    * @see SocketServiceHandler()
    */
   virtual bool init(std::shared_ptr<SocketServiceHandler> socketServiceHandler,
                     int serverPort,
                     int maxConnections) noexcept override;
   
   /**
    *
    * @param maxConnections
    * @return
    */
   virtual int getKernelEvents(int maxConnections) noexcept override;
   
   /**
    *
    * @param eventIndex
    * @return
    */
   virtual int fileDescriptorForEventIndex(int eventIndex) noexcept override;
   
  /**
   *
   * @param fileDescriptor
   * @return
   */
   virtual bool addFileDescriptorForRead(int fileDescriptor) noexcept override;
   
  /**
   *
   * @param fileDescriptor
   * @return
   */
   virtual bool removeFileDescriptorFromRead(int fileDescriptor) noexcept override;
   
  /**
   *
   * @param eventIndex
   * @return
   */
   virtual bool isEventDisconnect(int eventIndex) noexcept override;
   
  /**
   *
   * @param eventIndex
   * @return
   */
   virtual bool isEventRead(int eventIndex) noexcept override;


   // copying not allowed
   KqueueServer(const KqueueServer&) = delete;
   KqueueServer(KqueueServer&&) = delete;
   KqueueServer& operator=(const KqueueServer&) = delete;
   KqueueServer& operator=(KqueueServer&&) = delete;
   
   
private:
#ifdef KQUEUE_SUPPORT
   struct kevent* m_events;
#endif
   int m_kqfd;
};


#endif
