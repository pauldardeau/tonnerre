#include "MessageRequestHandler.h"
#include "MessageHandler.h"
#include "BasicException.h"
#include "Message.h"
#include "Logger.h"
#include "Socket.h"

//******************************************************************************

MessageRequestHandler::MessageRequestHandler(std::shared_ptr<Socket> socket, MessageHandler* handler) :
   RequestHandler(socket),
   m_handler(handler)
{
   Logger::logInstanceCreate("MessageRequestHandler");
}

//******************************************************************************

MessageRequestHandler::MessageRequestHandler(std::shared_ptr<SocketRequest> socketRequest, MessageHandler* handler) :
   RequestHandler(socketRequest),
   m_handler(handler)
{
   Logger::logInstanceCreate("MessageRequestHandler");
}

//******************************************************************************

MessageRequestHandler::~MessageRequestHandler()
{
   Logger::logInstanceDestroy("MessageRequestHandler");
}

//******************************************************************************

void MessageRequestHandler::run()
{
   std::shared_ptr<Socket> socket(getSocket());
   MessageHandler* messageHandler = m_handler;
   
   if ((socket != nullptr) && (messageHandler != nullptr)) {
      std::shared_ptr<Message> requestMessage(Message::reconstruct(socket));
      if (requestMessage != nullptr) {
         const std::string& requestName = requestMessage->getRequestName();
         if (!requestName.empty()) {
            const Message::MessageType messageType = requestMessage->getType();
            Message responseMessage(requestName, messageType);
            if (messageType == Message::MessageType::KeyValues) {
               KeyValuePairs responsePayload;
               
               try {
                  messageHandler->handleKeyValuesMessage(requestMessage,
                                                         responseMessage,
                                                         requestName,
                                                         requestMessage->getKeyValuesPayload(),
                                                         responsePayload);
                  responseMessage.setKeyValuesPayload(responsePayload);
               }
               catch (const BasicException& be) 
               {
                  // BasicException caught
                  Logger::error("execption caught in handling message: " + be.whatString());
               }
               catch (const std::exception& e)
               {
                  // exception caught
                  Logger::error("exception caught in handling message: " + std::string(e.what()));
               }
               catch (...)
               {
                  // unknown exception caught
                  Logger::error("exception caught in handling message");
               }
            } else if (messageType == Message::MessageType::Text) {
               std::string responsePayload;
               
               try {
                  messageHandler->handleTextMessage(requestMessage,
                                                    responseMessage,
                                                    requestName,
                                                    requestMessage->getTextPayload(),
                                                    responsePayload);
                  responseMessage.setTextPayload(responsePayload);
               }
               catch (const BasicException& be) 
               {
                  // BasicException caught
                  Logger::error("execption caught in handling message: " + be.whatString());
               }
               catch (const std::exception& e)
               {
                  // exception caught
                  Logger::error("exception caught in handling message: " + std::string(e.what()));
               }
               catch (...)
               {
                  // unknown exception caught
                  Logger::error("exception caught in handling message");
               }
            }
            
            if (!socket->write(responseMessage.toString())) {
               Logger::error("writing response message to socket failed");
            }
            
         } else {
            // request name is empty
            Logger::error("request name is empty");
         }
      } else {
         // unable to reconstruct request message
         Logger::error("unable to reconstruct request message");
      }
   } else {
      if (socket == nullptr) {
         Logger::error("no socket provided");
      }
      
      if (messageHandler == nullptr) {
         Logger::error("no message handler provided");
      }
   }
}

//******************************************************************************
