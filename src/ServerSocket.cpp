// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#include <cstdio>
#include <cstring>

#include <errno.h>

#include "ServerSocket.h"
#include "Socket.h"
#include "BasicException.h"
#include "Logger.h"


static const int BACKLOG = 50;

typedef socklen_t SOCKLEN_T;


//******************************************************************************

bool ServerSocket::setReuseAddr(int socketFD) noexcept
{
   int val_to_set = 1;
   
   if (0 == ::setsockopt(socketFD,
                         SOL_SOCKET,
                         SO_REUSEADDR,
                         (char *) &val_to_set,
                         sizeof(val_to_set))) {
      return true;
   } else {
      return false;
   }
}

//******************************************************************************

bool ServerSocket::listen(int socketFD, int backlog) noexcept
{
   if (::listen(socketFD, backlog) != 0) {
      Logger::error("unable to listen on server socket");
      return false;
   } else {
      return true;
   }
}

//******************************************************************************

bool ServerSocket::bind(int socketFD, int port) noexcept
{
   struct sockaddr_in serverAddr;
   ::memset((void *) &serverAddr, 0, sizeof(serverAddr));
   
   serverAddr.sin_family = AF_INET;
   serverAddr.sin_port = htons(port);
   serverAddr.sin_addr.s_addr = INADDR_ANY;
   
   const int rc = ::bind(socketFD,
                         (struct sockaddr*) &serverAddr,
                         sizeof(serverAddr));
   
   if (rc < 0) {
      Logger::error("unable to bind server socket to port");
      std::string errMsg;
      
      switch(errno) {
         case EACCES:
            errMsg = "EACCES";
            break;
         case EADDRINUSE:
            errMsg = "EADDRINUSE";
            break;
         case EADDRNOTAVAIL:
            errMsg = "EADDRNOTAVAIL";
            break;
         case EAFNOSUPPORT:
            errMsg = "EAFNOSUPPORT";
            break;
         case EBADF:
            errMsg = "EBADF";
            break;
         case EDESTADDRREQ:
            errMsg = "EDESTADDRREQ";
            break;
         case EFAULT:
            errMsg = "EFAULT";
            break;
         case EINVAL:
            errMsg = "EINVAL";
            break;
         case ENOTSOCK:
            errMsg = "ENOTSOCK";
            break;
         case EOPNOTSUPP:
            errMsg = "EOPNOTSUPP";
            break;
         default:
            errMsg = "<other>";
            break;
      }
      printf("errno = %d (%s)\n", errno, errMsg.c_str());
      return false;
   } else {
      return true;
   }
}

//******************************************************************************

ServerSocket::ServerSocket(int port) :
   m_serverSocket(-1),
   m_port(port)
{
   Logger::logInstanceCreate("ServerSocket");

   if (!create()) {
      throw BasicException("unable to create server socket");
   }

   if (!listen()) {
      close();
      throw BasicException("unable to listen on server socket");
   }
}

//******************************************************************************

ServerSocket::~ServerSocket() noexcept
{
   Logger::logInstanceDestroy("ServerSocket");

   close();
}

//******************************************************************************

bool ServerSocket::create() noexcept
{
   m_serverSocket = Socket::createSocket();

   if (m_serverSocket < 0) {
      Logger::error("unable to create server socket");
      return false;
   }

   ServerSocket::setReuseAddr(m_serverSocket);
   return ServerSocket::bind(m_serverSocket, m_port);
}

//******************************************************************************

bool ServerSocket::listen() noexcept
{
   return ServerSocket::listen(m_serverSocket, BACKLOG);
}

//******************************************************************************

std::shared_ptr<Socket> ServerSocket::accept() noexcept
{
   struct sockaddr_in clientAddr;
   SOCKLEN_T namelen = sizeof(clientAddr);

   const int connectionSocket = ::accept(m_serverSocket,
                                         (struct sockaddr*) &clientAddr,
                                         &namelen);

   if (connectionSocket < 0) {
      return nullptr;
   } else {
      return std::shared_ptr<Socket>(new Socket(connectionSocket));
   }
}

//******************************************************************************

void ServerSocket::close() noexcept
{
   m_serverSocket = -1;
}

//******************************************************************************




