// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef EPOLLSERVER_H
#define EPOLLSERVER_H

#include "KernelEventServer.h"


class SocketServiceHandler;
class Mutex;

#ifdef __linux__
#include <sys/epoll.h>
#define EPOLL_SUPPORT 1
#endif



/**
 * EpollServer is a wrapper for working with the epoll API. The epoll API
 * is a high-performance kernel event mechanism on Linux.
 */
class EpollServer : public KernelEventServer
{
public:
   /**
    * Determines if the Epoll mechanism is supported on the current platform
    * @return boolean indicating if epoll mechanism is supported
    */
   static bool isSupportedPlatform() noexcept;

   /**
    *
    * @param fdMutex
    * @param hwmConnectionsMutex
    */
   EpollServer(Mutex& fdMutex, Mutex& hwmConnectionsMutex) noexcept;
   
   /**
    * Destructor
    */
   ~EpollServer() noexcept;
   
   // KernelEventServer
   /**
    *
    * @param socketServiceHandler
    * @param serverPort
    * @param maxConnections
    * @return
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
   EpollServer(const EpollServer&) = delete;
   EpollServer(EpollServer&&) = delete;
   EpollServer& operator=(const EpollServer&) = delete;
   EpollServer& operator=(EpollServer&&) = delete;

   
private:
#ifdef EPOLL_SUPPORT
   struct epoll_event* m_events;
#endif
   int m_epfd;
};


#endif
