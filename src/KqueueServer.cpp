// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

#include <string>

#include "KqueueServer.h"
#include "Logger.h"


//******************************************************************************

bool KqueueServer::isSupportedPlatform() noexcept
{
#ifdef KQUEUE_SUPPORT
   return true;
#else
   return false;
#endif
}

//******************************************************************************

KqueueServer::KqueueServer(Mutex& fdMutex, Mutex& hwmConnectionsMutex) noexcept :
   KernelEventServer(fdMutex, hwmConnectionsMutex, "KqueueServer"),
#ifdef KQUEUE_SUPPORT
   m_events(nullptr),
#endif
   m_kqfd(-1)
{
   Logger::logInstanceCreate("KqueueServer");
}

//******************************************************************************

KqueueServer::~KqueueServer() noexcept
{
   Logger::logInstanceDestroy("KqueueServer");

#ifdef KQUEUE_SUPPORT
   if (nullptr != m_events) {
      free(m_events);
      m_events = nullptr;
   }
#endif
   
   if (-1 != m_kqfd) {
      ::close(m_kqfd);
   }
}

//******************************************************************************

bool KqueueServer::init(std::shared_ptr<SocketServiceHandler> socketServiceHandler,
                        int serverPort,
                        int maxConnections) noexcept
{
#ifndef KQUEUE_SUPPORT
   return false;
#endif

#ifdef KQUEUE_SUPPORT
   if (KernelEventServer::init(socketServiceHandler, serverPort, maxConnections)) {
      m_kqfd = ::kqueue();
      if (m_kqfd == -1) {
         Logger::critical("kqueue create failed");
         return false;
      }
      
      m_events = (struct kevent*) ::calloc(maxConnections, sizeof(struct kevent));
      
      // add our listener socket (server socket) as one of the fd's that
      // we want watched
      struct kevent ev;
      EV_SET(&ev,
             getListenerSocketFileDescriptor(),
             EVFILT_READ,
             EV_ADD,
             0, 0, nullptr);
      
      if (::kevent(m_kqfd, &ev, 1, 0, 0, nullptr) < 0) {
         Logger::critical("kevent failed for adding read filter");
         return false;
      } else {
         return true;
      }
   }
#endif
   
   return false;
}

//******************************************************************************

int KqueueServer::getKernelEvents(int maxConnections) noexcept
{
#ifdef KQUEUE_SUPPORT
   const int numberEventsReturned =
      ::kevent(m_kqfd,
               nullptr, 0,
               m_events, maxConnections,
               (struct timespec*) 0);
   if (-1 == numberEventsReturned) {
      Logger::critical("unable to retrieve events from kevent");
      return -1;
   }
   
   return numberEventsReturned;
#else
   return 0;
#endif
}

//******************************************************************************

int KqueueServer::fileDescriptorForEventIndex(int eventIndex) noexcept
{
   int client_fd = -1;
   
#ifdef KQUEUE_SUPPORT
   struct kevent current_event;
   current_event = m_events[eventIndex];
   client_fd = (int) current_event.ident;
#endif
   
   return client_fd;
}

//******************************************************************************

bool KqueueServer::addFileDescriptorForRead(int fileDescriptor) noexcept
{
#ifdef KQUEUE_SUPPORT
   struct kevent ev;
   EV_SET(&ev, fileDescriptor, EVFILT_READ, EV_ADD, 0, 0, nullptr);
   
   if (::kevent(m_kqfd, &ev, 1, nullptr, 0, nullptr) < 0) {
      Logger::critical("kevent failed adding read filter");
   } else {
      return true;
   }
#endif
   
   return false;
}

//******************************************************************************

bool KqueueServer::removeFileDescriptorFromRead(int fileDescriptor) noexcept
{
#ifdef KQUEUE_SUPPORT
   struct kevent ev;
   EV_SET(&ev, fileDescriptor, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
   
   if (::kevent(m_kqfd, &ev, 1, nullptr, 0, nullptr) < 0) {
      Logger::warning("kevent failed to delete read filter");
      return false;
   } else {
      return true;
   }
#endif

   return false;
}

//******************************************************************************

bool KqueueServer::isEventDisconnect(int eventIndex) noexcept
{
#ifdef KQUEUE_SUPPORT
   struct kevent current_event;
   current_event = m_events[eventIndex];
   return current_event.flags & EV_EOF;
#endif

   return false;
}

//******************************************************************************

bool KqueueServer::isEventRead(int eventIndex) noexcept
{
#ifdef KQUEUE_SUPPORT
   struct kevent current_event;
   current_event = m_events[eventIndex];
   return current_event.filter == EVFILT_READ;
#endif

   return false;
}

//******************************************************************************

