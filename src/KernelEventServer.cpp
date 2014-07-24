// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <string>

#include "KernelEventServer.h"
#include "Socket.h"
#include "SocketServiceHandler.h"
#include "SocketRequest.h"
#include "MutexLock.h"
#include "Logger.h"
#include "ServerSocket.h"
#include "BasicException.h"



//******************************************************************************

KernelEventServer::KernelEventServer(Mutex& fdMutex,
                                     Mutex& hwmConnectionsMutex,
                                     const std::string& serverName) noexcept :
   m_socketServiceHandler(nullptr),
   m_listBusyFlags(nullptr),
   m_fdMutex(fdMutex),
   m_hwmConnectionsMutex(hwmConnectionsMutex),
   m_maxConcurrentRequests(0L),
   m_concurrentRequests(0L),
   m_maxConcurrentConnections(0L),
   m_concurrentConnections(0L),
   m_serverPort(0),
   m_maxConnections(0),
   m_listenBacklog(10),
   m_listenerFD(-1),
   m_kqfd(-1),
   m_numberEventsReturned(0)
{
}

//******************************************************************************

KernelEventServer::~KernelEventServer() noexcept
{
   if (nullptr != m_listBusyFlags) {
      free(m_listBusyFlags);
      m_listBusyFlags = nullptr;
   }
   
   if (-1 != m_listenerFD) {
      ::close(m_listenerFD);
   }
}

//******************************************************************************

bool KernelEventServer::init(std::shared_ptr<SocketServiceHandler> socketServiceHandler,
                             int serverPort,
                             int maxConnections) noexcept
{
   m_socketServiceHandler = std::move(socketServiceHandler);
   m_serverPort = serverPort;
   m_maxConnections = maxConnections;
   
   if (nullptr == m_socketServiceHandler) {
      Logger::critical("no socket service handler set");
      return false;
   }
   
   if (m_serverPort <= 0) {
      Logger::critical("serverPort must be positive, non-zero value");
      return false;
   }
   
   if (m_maxConnections <= 0) {
      Logger::critical("maxConnections must be positive, non-zero value");
      return false;
   }
   
   
   m_listenerFD = Socket::createSocket();
   if (m_listenerFD == -1) {
      Logger::critical("error: unable to create server listening socket");
      return false;
   }
   
   if (!ServerSocket::setReuseAddr(m_listenerFD)) {
      Logger::critical("unable to set REUSEADDR for socket");
      return false;
   }
   
   if (!ServerSocket::bind(m_listenerFD, m_serverPort)) {
      Logger::critical("bind failed");
      return false;
   }
   
   if (!ServerSocket::listen(m_listenerFD, m_listenBacklog)) {
      Logger::critical("listen failed");
      return false;
   }
   
   m_fdmax = m_listenerFD; // so far, it's this one
   m_listBusyFlags = (bool*) ::calloc(m_maxConnections, sizeof(bool));
   
   m_maxConcurrentConnections = 0L;
   m_concurrentConnections = 0L;
   
   return true;
}

//******************************************************************************

void KernelEventServer::run() noexcept
{
   struct sockaddr_in clientaddr;
   socklen_t addrlen = sizeof(clientaddr);
   int newfd;
   char msg[128];
   
   const std::string& handlerName = m_socketServiceHandler->getName();
   
   Logger::info(std::string("using handler: ") + handlerName);
   
   for (;;) {
      
      const bool isLoggingDebug = Logger::isLogging(Logger::LogLevel::Debug);
      
      m_numberEventsReturned = getKernelEvents(m_maxConnections);
      
      if (isLoggingDebug) {
         std::snprintf(msg, 128, "KernelEventServer::run, numberEventsReturned = %d",
                  m_numberEventsReturned);
         Logger::debug(msg);
      }
      
      if (m_numberEventsReturned < 1) {
         continue;
      }
      
      for (int index = 0; index < m_numberEventsReturned; ++index) {
         
         int client_fd = fileDescriptorForEventIndex(index);
         
         if (isLoggingDebug) {
            std::snprintf(msg, 128, "KernelEventServer::run waiting for locks fd=%d",
                     client_fd);
            Logger::debug(msg);
         }
         
         //m_fdMutex.lock();
         //m_hwmConnectionsMutex.lock();
         
         if (isLoggingDebug) {
            Logger::debug("KernelEventServer::run have locks");
         }
         
         if (client_fd == m_listenerFD) {
            newfd = ::accept(m_listenerFD, (struct sockaddr *)&clientaddr, &addrlen);
            if (newfd == -1) {
               Logger::warning("server accept failed");
            } else {
               if (isLoggingDebug) {
                  std::snprintf(msg, 128, "client %d connected", newfd);
                  Logger::debug(msg);
               }
               
               {
                  ++m_concurrentConnections;
                  if (m_concurrentConnections > m_maxConcurrentConnections) {
                     // new high water mark
                     m_maxConcurrentConnections = m_concurrentConnections;
                     
                     if (Logger::isLogging(Logger::LogLevel::Info)) {
                        std::snprintf(msg, 128, "new hwm concurrent connections = %ld",
                                 m_maxConcurrentConnections);
                        Logger::info(msg);
                     }
                  }
               }
               
               if (!addFileDescriptorForRead(newfd)) {
                  Logger::critical("kernel event server failed adding read filter");
               }
            }
         } else {
            if (isEventDisconnect(index)) {
               if (isLoggingDebug) {
                  std::snprintf(msg, 128, "client disconnected %d", client_fd);
                  Logger::debug(msg);
               }
               
               --m_concurrentConnections;
               
               // was it busy?
               if (m_listBusyFlags[index]) {
                  m_listBusyFlags[index] = false;
                  --m_concurrentRequests;
               }
               
               if (!removeFileDescriptorFromRead(client_fd)) {
                  Logger::warning("kernel event server failed to delete read filter");
               }
               
               ::close(client_fd);
            } else if (isEventRead(index)) {
               
               // are we already busy with this socket?
               const bool isAlreadyBusy = m_listBusyFlags[index];
               
               if (!isAlreadyBusy) {
                  if (isLoggingDebug) {
                     std::snprintf(msg, 128, "handling read for socket %d", client_fd);
                     Logger::debug(msg);
                  }
                  
                  ++m_concurrentRequests;
                  if (m_concurrentRequests > m_maxConcurrentRequests) {
                     // new high water mark
                     m_maxConcurrentRequests = m_concurrentRequests;
                     if (Logger::isLogging(Logger::LogLevel::Info)) {
                        std::snprintf(msg, 128, "new hwm concurrent requests = %ld",
                                 m_maxConcurrentRequests);
                        Logger::info(msg);
                     }
                  }
                  
                  
                  // remove file descriptor from watch
                  if (isLoggingDebug) {
                     std::snprintf(msg, 128, "removing socket from watch for read (%d)",
                              client_fd);
                     Logger::debug(msg);
                  }

                  if (!removeFileDescriptorFromRead(client_fd)) {
                     Logger::error("unable to remove file descriptor from read");
                  }
                  
                  m_listBusyFlags[index] = true;
                  
                  //TODO: grab stats (connections, requests)
                  
                  // give up our lock
                  if (isLoggingDebug) {
                     std::snprintf(msg, 128, "giving up locks and dispatching request for socket %d",
                              client_fd);
                     Logger::debug(msg);
                  }
                  
                  //m_fdMutex.unlock();
                  //m_hwmConnectionsMutex.unlock();

                  std::shared_ptr<Socket> clientSocket(new Socket(this, client_fd));
                  clientSocket->setUserIndex(index);
                  //TODO: stuff the stats in the socket or the socketrequest

                  std::shared_ptr<SocketRequest> socketRequest(new SocketRequest(clientSocket, m_socketServiceHandler));

                  try
                  {
                     m_socketServiceHandler->serviceSocket(socketRequest);
                  }
                  catch (const BasicException& be)
                  {
                     Logger::error("exception in serviceSocket on handler: " + be.whatString());
                  }
                  catch (const std::exception& e)
                  {
                     Logger::error("exception in serviceSocket on handler: " + std::string(e.what()));
                  }
                  catch (...)
                  {
                     Logger::error("exception in serviceSocket on handler");
                  }
                  
                  //socketRequest->run();
                  
                  m_listBusyFlags[index] = false;
               } else {
                  std::snprintf(msg, 128, "already busy with socket %d", client_fd);
                  Logger::warning(msg);
               }
            }
         }
         
         //m_fdMutex.unlock();
         //m_hwmConnectionsMutex.unlock();
         
         if (isLoggingDebug) {
            Logger::debug("KernelEventServer::run finished iteration of innner loop");
         }
         
      }
   }  // for (;;)
}

//******************************************************************************

void KernelEventServer::notifySocketComplete(std::shared_ptr<Socket> socket) noexcept
{
   char msg[128];
   const bool isLoggingDebug = Logger::isLogging(Logger::LogLevel::Debug);
   
   const int socketFD = socket->getFileDescriptor();
   
   if (isLoggingDebug) {
      std::snprintf(msg, 128, "completed request with socket %d", socketFD);
      Logger::debug(msg);
   }
   
   const int userIndex = socket->getUserIndex();
   
   if (isLoggingDebug) {
      Logger::debug("notifySocketComplete: waiting for locks");
   }
   
   //MutexLock lock(m_fdMutex);
   //MutexLock lockConnections(m_hwmConnectionsMutex);
   
   if (isLoggingDebug) {
      Logger::debug("notifySocketComplete: have locks");
   }
   
   // mark the fd as not being busy anymore
   m_listBusyFlags[userIndex] = false;
   --m_concurrentRequests;
   
   if (!socket->isConnected()) {
      
      --m_concurrentConnections;
      
      // remove file descriptor and close socket
      if (isLoggingDebug) {
         std::snprintf(msg, 128, "notifySocketComplete: client disconnect %d",
                  socketFD);
         Logger::debug(msg);

         std::snprintf(msg, 128, "removing file descriptor %d", socketFD);
         Logger::debug(msg);
      }
      
      if (!removeFileDescriptorFromRead(socketFD)) {
         Logger::error("unable to remove file descriptor from read");
      }
      
      ::close(socketFD);
   } else {
      // it's still connected
      
      if (isLoggingDebug) {
         std::snprintf(msg, 128, "adding socket back to watch for read (%d)",
                  socketFD);
         Logger::debug(msg);
      }

      // add socket back to watch
      if (!addFileDescriptorForRead(socketFD)) {
         Logger::critical("kernel event add read filter failed");
      }
   }
}

//******************************************************************************

int KernelEventServer::getListenerSocketFileDescriptor() const noexcept
{
   return m_listenerFD;
}

//******************************************************************************

