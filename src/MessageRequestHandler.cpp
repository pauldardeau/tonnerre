#include "MessageRequestHandler.h"
#include "MessageHandler.h"
#include "BasicException.h"
#include "Message.h"

//******************************************************************************

MessageRequestHandler::MessageRequestHandler(SocketServer& socketServer, std::shared_ptr<Socket> socket) :
   RequestHandler(socket),
   m_socketServer(socketServer)
{
}

//******************************************************************************

MessageRequestHandler::MessageRequestHandler(SocketServer& socketServer, std::shared_ptr<SocketRequest> socketRequest) :
   RequestHandler(socketRequest),
   m_socketServer(socketServer)
{
}

//******************************************************************************

void MessageRequestHandler::run()
{
   std::shared_ptr<Socket> socket(getSocket());
   MessageHandler* messageHandler = nullptr;
   
   if ((socket != nullptr) && (messageHandler != nullptr)) {
      std::shared_ptr<Message> requestMessage(Message::reconstruct(socket));
      if (requestMessage != nullptr) {
         const std::string& requestName = requestMessage->getRequestName();
         if (!requestName.empty()) {
            const Message::MessageType messageType = requestMessage->getType();
            Message responseMessage(messageType);
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
               }
               catch (const std::exception& e)
               {
                  // exception caught
               }
               catch (...)
               {
                  // unknown exception caught
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
               }
               catch (const std::exception& e)
               {
                  // exception caught
               }
               catch (...)
               {
                  // unknown exception caught
               }
            }
         } else {
            // request name is empty
         }
         
      } else {
         // unable to reconstruct request message
      }
   } else {
      if (socket == nullptr) {
         
      }
      
      if (messageHandler == nullptr) {
         
      }
   }
}

//******************************************************************************
