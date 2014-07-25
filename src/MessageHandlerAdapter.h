// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef MESSAGEHANDLERADAPTER_H
#define MESSAGEHANDLERADAPTER_H

#include "MessageHandler.h"

class MessageHandlerAdapter : public MessageHandler
{
public:
   virtual ~MessageHandlerAdapter() {}
   
   virtual void handleTextMessage(std::shared_ptr<Message> requestMessage,
                                  Message& responseMessage,
                                  const std::string& requestName,
                                  const std::string& requestPayload,
                                  std::string& responsePayload) {}
   
   virtual void handleKeyValuesMessage(std::shared_ptr<Message> requestMessage,
                                       Message& responseMessage,
                                       const std::string& requestName,
                                       const KeyValuePairs& requestPayload,
                                       KeyValuePairs& responsePayload) {}
};

#endif
