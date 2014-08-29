// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#include <memory>

#include "MessageSocketServiceHandler.h"
#include "SocketRequest.h"
#include "MessageRequestHandler.h"
#include "Logger.h"

using namespace tonnerre;
using namespace chaudiere;

const std::string MessageSocketServiceHandler::handlerName = "MessageSocketServiceHandler";

//******************************************************************************

MessageSocketServiceHandler::MessageSocketServiceHandler(MessageHandler* handler) :
   m_handler(handler)
{
   Logger::logInstanceCreate("MessageSocketServiceHandler");
}

//******************************************************************************

MessageSocketServiceHandler::~MessageSocketServiceHandler()
{
   Logger::logInstanceDestroy("MessageSocketServiceHandler");
}

//******************************************************************************

void MessageSocketServiceHandler::serviceSocket(std::shared_ptr<SocketRequest> socketRequest)
{
   MessageRequestHandler messageRequestHandler(socketRequest, m_handler);
   messageRequestHandler.run();
}

//******************************************************************************

const std::string& MessageSocketServiceHandler::getName() const
{
   return handlerName;
}

//******************************************************************************
