// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#include "MessageRequestHandler.h"
#include "MessageHandler.h"
#include "BasicException.h"
#include "Message.h"
#include "Logger.h"
#include "Socket.h"
#include "KeyValuePairs.h"

using namespace tonnerre;
using namespace chaudiere;

//******************************************************************************

MessageRequestHandler::MessageRequestHandler(Socket* socket, MessageHandler* handler) :
   RequestHandler(socket),
   m_handler(handler) {
   Logger::logInstanceCreate("MessageRequestHandler");
}

//******************************************************************************

MessageRequestHandler::MessageRequestHandler(SocketRequest* socketRequest,
                                             MessageHandler* handler) :
   RequestHandler(socketRequest),
   m_handler(handler) {
   Logger::logInstanceCreate("MessageRequestHandler");
}

//******************************************************************************

MessageRequestHandler::~MessageRequestHandler() {
   Logger::logInstanceDestroy("MessageRequestHandler");
}

//******************************************************************************

void MessageRequestHandler::run() {
   Socket* socket(getSocket());
   MessageHandler* messageHandler = m_handler;
   
   if ((socket != NULL) && (messageHandler != NULL)) {
      Message* requestMessage(Message::reconstruct(socket));
      if (requestMessage != NULL) {
         const std::string& requestName = requestMessage->getRequestName();
         if (!requestName.empty()) {
            const MessageType messageType = requestMessage->getType();
            Message responseMessage(requestName, messageType);
            if (messageType == MessageTypeKeyValues) {
               KeyValuePairs responsePayload;
               
               try {
                  messageHandler->handleKeyValuesMessage(*requestMessage,
                                                         responseMessage,
                                                         requestName,
                                                         requestMessage->getKeyValuesPayload(),
                                                         responsePayload);
                  responseMessage.setKeyValuesPayload(responsePayload);
               } catch (const BasicException& be) {
                  // BasicException caught
                  Logger::error("execption caught in handling message: " + be.whatString());
               } catch (const std::exception& e) {
                  // exception caught
                  Logger::error("exception caught in handling message: " + std::string(e.what()));
               } catch (...) {
                  // unknown exception caught
                  Logger::error("exception caught in handling message");
               }
            } else if (messageType == MessageTypeText) {
               std::string responsePayload;
               
               try {
                  messageHandler->handleTextMessage(*requestMessage,
                                                    responseMessage,
                                                    requestName,
                                                    requestMessage->getTextPayload(),
                                                    responsePayload);
                  responseMessage.setTextPayload(responsePayload);
               } catch (const BasicException& be) {
                  // BasicException caught
                  Logger::error("execption caught in handling message: " + be.whatString());
               } catch (const std::exception& e) {
                  // exception caught
                  Logger::error("exception caught in handling message: " + std::string(e.what()));
               } catch (...) {
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
      if (socket == NULL) {
         Logger::error("no socket provided");
      }
      
      if (messageHandler == NULL) {
         Logger::error("no message handler provided");
      }
   }
}

//******************************************************************************
