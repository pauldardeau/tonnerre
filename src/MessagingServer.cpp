#include "MessagingServer.h"
#include "MessageHandler.h"
#include "ServerSocket.h"
#include "MessageRequestHandler.h"
#include "Logger.h"

//******************************************************************************

MessagingServer::MessagingServer(const std::string& configFilePath,
                                 const std::string& serverServiceName,
                                 MessageHandler* handler) :
   SocketServer(configFilePath),
   m_handler(handler),
   m_serviceName(serverServiceName)
{
   Logger::logInstanceCreate("MessagingServer");
}

//******************************************************************************

MessagingServer::~MessagingServer()
{
   Logger::logInstanceDestroy("MessagingServer");
}

//******************************************************************************

std::shared_ptr<RequestHandler> MessagingServer::handlerForSocket(std::shared_ptr<Socket> socket)
{
   return std::shared_ptr<RequestHandler>(new MessageRequestHandler(*this, socket));
}

//******************************************************************************

std::shared_ptr<RequestHandler> MessagingServer::handlerForSocketRequest(std::shared_ptr<SocketRequest> socketRequest)
{
   return std::shared_ptr<RequestHandler>(new MessageRequestHandler(*this, socketRequest));
}

//******************************************************************************

