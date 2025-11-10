// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef TONNERRE_MESSAGEHANDLERADAPTER_H
#define TONNERRE_MESSAGEHANDLERADAPTER_H

#include "MessageHandler.h"
#include "KeyValuePairs.h"

namespace tonnerre
{

/**
 *
 */
class MessageHandlerAdapter : public MessageHandler
{
public:
   /**
    * Destructor
    */
   virtual ~MessageHandlerAdapter() {}

   /**
    *
    * @param requestMessage
    * @param responseMessage
    * @param requestName
    * @param requestPayload
    * @param responsePayload
    */
   virtual void handleTextMessage(const Message& requestMessage,
                                  Message& responseMessage,
                                  const std::string& requestName,
                                  const std::string& requestPayload,
                                  std::string& responsePayload) {}

   /**
    *
    * @param requestMessage
    * @param responseMessage
    * @param requestName
    * @param requestPayload
    * @param responsePayload
    */
   virtual void handleKeyValuesMessage(const Message& requestMessage,
                                       Message& responseMessage,
                                       const std::string& requestName,
                                       const chaudiere::KeyValuePairs& requestPayload,
                                       chaudiere::KeyValuePairs& responsePayload) {}
};

}

#endif
