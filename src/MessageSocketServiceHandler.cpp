// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#include "MessageSocketServiceHandler.h"
#include "SocketRequest.h"
#include "MessageRequestHandler.h"
#include "Logger.h"

const std::string MessageSocketServiceHandler::handlerName = "MessageSocketServiceHandler";

//******************************************************************************

MessageSocketServiceHandler::MessageSocketServiceHandler()
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
   MessageRequestHandler messageRequestHandler(socketRequest);
   messageRequestHandler.run();
}

//******************************************************************************

const std::string& MessageSocketServiceHandler::getName() const
{
   return handlerName;
}

//******************************************************************************
