// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef KqueueServer_h
#define KqueueServer_h

#include "KernelEventServer.h"

class SocketServiceHandler;
class Mutex;


//#if defined(__APPLE__) || defined(__FreeBSD__)
#ifndef __linux__
#define KQUEUE_SUPPORT 1
#include <sys/event.h>
#endif


/*!
 * KqueueServer is a wrapper for working with the kqueue API. The kqueue API
 * is a high-performance kernel event mechanism on FreeBSD and OSX.
 */
class KqueueServer : public KernelEventServer
{
public:
   static bool isSupportedPlatform() noexcept;
   
   KqueueServer(Mutex& fdMutex, Mutex& hwmConnectionsMutex) noexcept;
   ~KqueueServer() noexcept;
   
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
