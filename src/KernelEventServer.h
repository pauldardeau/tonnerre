// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef KERNELEVENTSERVER_H
#define KERNELEVENTSERVER_H

#include <memory>

#include "Socket.h"
#include "SocketCompletionObserver.h"

class Mutex;
class SocketServiceHandler;


/**
 * KernelEventServer is an abstract base class for kernel event server
 * mechanisms such as kqueue and epoll.
 */
class KernelEventServer : public SocketCompletionObserver
{
public:
   /**
    *
    * @param fdMutex
    * @param hwmConnectionsMutex
    * @param serverName
    */
   KernelEventServer(Mutex& fdMutex,
                     Mutex& hwmConnectionsMutex,
                     const std::string& serverName) noexcept;
                     
   /**
    * Destructor
    */
   virtual ~KernelEventServer() noexcept;
   
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
                     int maxConnections) noexcept;
                     
   /**
    *
    */
   virtual void run() noexcept;

  /**
   *
   * @param maxConnections
   * @return
   */
   virtual int getKernelEvents(int maxConnections) noexcept = 0;

  /**
   *
   * @param eventIndex
   * @return
   */
   virtual int fileDescriptorForEventIndex(int eventIndex) noexcept = 0;
   
  /**
   *
   * @param fileDescriptor
   * @return
   */
   virtual bool addFileDescriptorForRead(int fileDescriptor) noexcept = 0;
   
  /**
   *
   * @param fileDescriptor
   * @return
   */
   virtual bool removeFileDescriptorFromRead(int fileDescriptor) noexcept = 0;

  /**
   *
   * @param eventIndex
   * @return
   */
   virtual bool isEventDisconnect(int eventIndex) noexcept = 0;
   
   /**
    *
    * @param eventIndex
    * @return
    */
   virtual bool isEventRead(int eventIndex) noexcept = 0;
   
   // SocketCompletionObserver
   /**
    *
    * @param socket
    */
   void notifySocketComplete(std::shared_ptr<Socket> socket) noexcept override;

   // copying not allowed
   KernelEventServer(const KernelEventServer&) = delete;
   KernelEventServer(KernelEventServer&&) = delete;
   KernelEventServer& operator=(const KernelEventServer&) = delete;
   KernelEventServer& operator=(KernelEventServer&&) = delete;

   
protected:
   /**
    *
    * @return
    */
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

#endif
