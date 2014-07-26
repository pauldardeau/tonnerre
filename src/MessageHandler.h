// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef MESSAGEHANDLER_H
#define MESSAGEHANDLER_H

#include <string>

class KeyValuePairs;
class Message;


/**
 * MessageHandler is the interface (abstract based class) to use for all
 * custom message handlers.
 */
class MessageHandler
{
public:
   virtual ~MessageHandler() {}
   
   /**
    * Handles a message with a text payload (Text type)
    * @param requestMessage the request message
    * @param responseMessage the response message
    * @param requestName the name of the request
    * @param requestPayload the request payload text
    * @param responsePayload the response payload text
    * @see Message()
    */
   virtual void handleTextMessage(std::shared_ptr<Message> requestMessage,
                                  Message& responseMessage,
                                  const std::string& requestName,
                                  const std::string& requestPayload,
                                  std::string& responsePayload) = 0;

   /**
    * Handles a message with a payload of key-value pairs (KeyValues type)
    * @param requestMessage the request message
    * @param responseMessage the response message
    * @param requestName the name of the request
    * @param requestPayload the request payload as key-value pairs
    * @param responsePayload the response payload as key-value pairs
    * @see Message()
    * @see KeyValuePairs()
    */
   virtual void handleKeyValuesMessage(std::shared_ptr<Message> requestMessage,
                                       Message& responseMessage,
                                       const std::string& requestName,
                                       const KeyValuePairs& requestPayload,
                                       KeyValuePairs& responsePayload) = 0;
   
};

#endif
