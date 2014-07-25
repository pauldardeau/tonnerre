// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef MESSAGESOCKETSERVICEHANDLER_H
#define MESSAGESOCKETSERVICEHANDLER_H

#include <string>

#include "SocketServiceHandler.h"

class MessageHandler;
class SocketRequest;


class MessageSocketServiceHandler : public SocketServiceHandler
{
public:
   MessageSocketServiceHandler(MessageHandler* handler);
   virtual ~MessageSocketServiceHandler();
   
   virtual void serviceSocket(std::shared_ptr<SocketRequest> socketRequest);
   
   /**
    * Retrieves the name of the handler. This is primarily an aid for debugging.
    * @return the name of the handler
    */
   virtual const std::string& getName() const;
   
private:
   static const std::string handlerName;
   MessageHandler* m_handler;

};

#endif
