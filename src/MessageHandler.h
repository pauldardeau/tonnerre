#ifndef MESSAGEHANDLER_H
#define MESSAGEHANDLER_H

#include <string>

class KeyValuePairs;
class Message;


class MessageHandler
{
public:
   virtual ~MessageHandler() {}
   
   virtual void handleTextMessage(std::shared_ptr<Message> requestMessage,
                                  Message& responseMessage,
                                  const std::string& requestName,
                                  const std::string& requestPayload,
                                  std::string& responsePayload) = 0;
   
   virtual void handleKeyValuesMessage(std::shared_ptr<Message> requestMessage,
                                       Message& responseMessage,
                                       const std::string& requestName,
                                       const KeyValuePairs& requestPayload,
                                       KeyValuePairs& responsePayload) = 0;
   
};

#endif
