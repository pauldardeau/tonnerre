// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#include "MessagingServer.h"
#include "MessageHandler.h"
#include "ServerSocket.h"
#include "MessageRequestHandler.h"
#include "MessageSocketServiceHandler.h"
#include "Logger.h"

using namespace std;
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
   m_serviceName(serverServiceName) {
   Logger::logInstanceCreate("MessagingServer");
}

//******************************************************************************

MessagingServer::~MessagingServer() {
   Logger::logInstanceDestroy("MessagingServer");
}

//******************************************************************************

void MessagingServer::setMessageHandler(MessageHandler* handler) {
   m_handler = handler;
}

//******************************************************************************

RequestHandler* MessagingServer::handlerForSocket(Socket* socket) {
   return new MessageRequestHandler(socket, m_handler);
}

//******************************************************************************

RequestHandler* MessagingServer::handlerForSocketRequest(SocketRequest* socketRequest) {
   return new MessageRequestHandler(socketRequest, m_handler);
}

//******************************************************************************

SocketServiceHandler* MessagingServer::createSocketServiceHandler() {
   return new MessageSocketServiceHandler(m_handler);
}

//******************************************************************************
