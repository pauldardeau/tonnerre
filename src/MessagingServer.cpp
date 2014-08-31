// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#include "MessagingServer.h"
#include "MessageHandler.h"
#include "ServerSocket.h"
#include "MessageRequestHandler.h"
#include "MessageSocketServiceHandler.h"
#include "Logger.h"

using namespace tonnerre;
using namespace chaudiere;

const std::string MessagingServer::SERVER_NAME     = "Tonnerre";
const std::string MessagingServer::SERVER_VERSION  = "0.1";

//******************************************************************************

MessagingServer::MessagingServer(const std::string& configFilePath,
                                 const std::string& serverServiceName,
                                 MessageHandler* handler) :
   SocketServer(SERVER_NAME, SERVER_VERSION, configFilePath),
   m_handler(handler),
   m_serviceName(serverServiceName)
{
   Logger::logInstanceCreate("MessagingServer");
}

//******************************************************************************

MessagingServer::~MessagingServer() noexcept
{
   Logger::logInstanceDestroy("MessagingServer");
}

//******************************************************************************

void MessagingServer::setMessageHandler(MessageHandler* handler)
{
   m_handler = handler;
}

//******************************************************************************

std::shared_ptr<RequestHandler> MessagingServer::handlerForSocket(std::shared_ptr<Socket> socket)
{
   return std::shared_ptr<RequestHandler>(new MessageRequestHandler(socket, m_handler));
}

//******************************************************************************

std::shared_ptr<RequestHandler> MessagingServer::handlerForSocketRequest(std::shared_ptr<SocketRequest> socketRequest)
{
   return std::shared_ptr<RequestHandler>(new MessageRequestHandler(socketRequest, m_handler));
}

//******************************************************************************

std::shared_ptr<SocketServiceHandler> MessagingServer::createSocketServiceHandler()
{
   return std::shared_ptr<SocketServiceHandler>(new MessageSocketServiceHandler(m_handler));
}

//******************************************************************************
