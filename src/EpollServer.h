// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef C10KServer_EpollServer_h
#define C10KServer_EpollServer_h

#include "KernelEventServer.h"


class SocketServiceHandler;
class Mutex;

#ifdef __linux__
#include <sys/epoll.h>
#define EPOLL_SUPPORT 1
#endif



/*!
 * EpollServer is a wrapper for working with the epoll API. The epoll API
 * is a high-performance kernel event mechanism on Linux.
 */
class EpollServer : public KernelEventServer
{
public:
   static bool isSupportedPlatform() noexcept;

   EpollServer(Mutex& fdMutex, Mutex& hwmConnectionsMutex) noexcept;
   ~EpollServer() noexcept;
   
   // KernelEventServer
   virtual bool init(std::shared_ptr<SocketServiceHandler> socketServiceHandler,
                     int serverPort,
                     int maxConnections) noexcept override;
   
   virtual int getKernelEvents(int maxConnections) noexcept override;
   virtual int fileDescriptorForEventIndex(int eventIndex) noexcept override;
   virtual bool addFileDescriptorForRead(int fileDescriptor) noexcept override;
   virtual bool removeFileDescriptorFromRead(int fileDescriptor) noexcept override;
   virtual bool isEventDisconnect(int eventIndex) noexcept override;
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
