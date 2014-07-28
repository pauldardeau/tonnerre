// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef MESSAGESOCKETSERVICEHANDLER_H
#define MESSAGESOCKETSERVICEHANDLER_H

#include <string>

#include "SocketServiceHandler.h"
#include "SocketRequest.h"


namespace tonnerre
{
   class MessageHandler;

/**
 *
 */
class MessageSocketServiceHandler : public chaudiere::SocketServiceHandler
{
public:
   /**
    *
    * @param handler
    * @see MessageHandler()
    */
   MessageSocketServiceHandler(MessageHandler* handler);
   
   /**
    * Destructor
    */
   virtual ~MessageSocketServiceHandler();
   
   /**
    *
    * @param socketRequest
    * @see SocketRequest()
    */
   virtual void serviceSocket(std::shared_ptr<chaudiere::SocketRequest> socketRequest);
   
   /**
    * Retrieves the name of the handler. This is primarily an aid for debugging.
    * @return the name of the handler
    */
   virtual const std::string& getName() const;
   
private:
   static const std::string handlerName;
   MessageHandler* m_handler;

};

}

#endif
