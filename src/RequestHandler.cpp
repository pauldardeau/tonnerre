// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#include "RequestHandler.h"
#include "Socket.h"
#include "SocketRequest.h"
#include "Thread.h"
#include "BasicException.h"
#include "Logger.h"
#include "StrUtils.h"


//******************************************************************************

RequestHandler::RequestHandler(std::shared_ptr<SocketRequest> socketRequest) noexcept :
   m_socket(nullptr),
   m_socketRequest(socketRequest),
   m_isThreadPooling(false)
{
   Logger::logInstanceCreate("RequestHandler");
}

//******************************************************************************

RequestHandler::RequestHandler(std::shared_ptr<Socket> socket) noexcept :
   m_socket(socket),
   m_socketRequest(nullptr),
   m_isThreadPooling(false)
{
   Logger::logInstanceCreate("RequestHandler");
}

//******************************************************************************

RequestHandler::~RequestHandler() noexcept
{
   Logger::logInstanceDestroy("RequestHandler");

   if (m_socket) {
      m_socket->close();
   }
}

//******************************************************************************

void RequestHandler::setThreadPooling(bool isThreadPooling) noexcept
{
   m_isThreadPooling = isThreadPooling;
}

//******************************************************************************

std::shared_ptr<Socket> RequestHandler::getSocket() noexcept
{
   std::shared_ptr<Socket> socket = nullptr;

   if (m_socket) {
      socket = m_socket;
   } else if (m_socketRequest) {
      socket = m_socketRequest->getSocket();
   }
   
   return socket;
}

//******************************************************************************
