// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef __HttpServer__KernelEventServer__
#define __HttpServer__KernelEventServer__

#include <memory>

#include "Socket.h"
#include "SocketCompletionObserver.h"

class Mutex;
class SocketServiceHandler;


/*!
 * KernelEventServer is an abstract base class for kernel event server
 * mechanisms such as kqueue and epoll.
 */
class KernelEventServer : public SocketCompletionObserver
{
public:
   KernelEventServer(Mutex& fdMutex,
                     Mutex& hwmConnectionsMutex,
                     const std::string& serverName) noexcept;
   virtual ~KernelEventServer() noexcept;
   
   virtual bool init(std::shared_ptr<SocketServiceHandler> socketServiceHandler,
                     int serverPort,
                     int maxConnections) noexcept;
   virtual void run() noexcept;
   
   virtual int getKernelEvents(int maxConnections) noexcept = 0;
   virtual int fileDescriptorForEventIndex(int eventIndex) noexcept = 0;
   virtual bool addFileDescriptorForRead(int fileDescriptor) noexcept = 0;
   virtual bool removeFileDescriptorFromRead(int fileDescriptor) noexcept = 0;
   virtual bool isEventDisconnect(int eventIndex) noexcept = 0;
   virtual bool isEventRead(int eventIndex) noexcept = 0;
   
   // SocketCompletionObserver
   void notifySocketComplete(std::shared_ptr<Socket> pSocket) noexcept override;

   // copying not allowed
   KernelEventServer(const KernelEventServer&) = delete;
   KernelEventServer(KernelEventServer&&) = delete;
   KernelEventServer& operator=(const KernelEventServer&) = delete;
   KernelEventServer& operator=(KernelEventServer&&) = delete;

   
protected:
   int getListenerSocketFileDescriptor() const noexcept;


private:
   std::shared_ptr<SocketServiceHandler> m_socketServiceHandler;
   bool* m_listBusyFlags;
   Mutex& m_fdMutex;
   Mutex& m_hwmConnectionsMutex;  // high water mark (concurrent connections)
   long m_maxConcurrentRequests;
   long m_concurrentRequests;
   long m_maxConcurrentConnections;
   long m_concurrentConnections;
   int m_serverPort;
   int m_maxConnections;
   int m_listenBacklog;
   int m_listenerFD;
   int m_kqfd;
   int m_fdmax;
   int m_numberEventsReturned;

};

#endif /* defined(__HttpServer__KernelEventServer__) */
