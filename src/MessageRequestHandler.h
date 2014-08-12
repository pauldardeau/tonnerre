// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef TONNERRE_MESSAGEREQUESTHANDLER_H
#define TONNERRE_MESSAGEREQUESTHANDLER_H

#include "RequestHandler.h"


namespace tonnerre
{
   class MessageHandler;

/**
 *
 */
class MessageRequestHandler : public chaudiere::RequestHandler
{
public:
   /**
    *
    * @param socket
    * @param handler
    * @see Socket()
    * @see MessageHandler()
    */
   MessageRequestHandler(std::shared_ptr<chaudiere::Socket> socket, MessageHandler* handler);
   
   /**
    *
    * @param socketRequest
    * @param handler
    * @see SocketRequest()
    * @see MessageHandler()
    */
   MessageRequestHandler(std::shared_ptr<chaudiere::SocketRequest> socketRequest, MessageHandler* handler);
   
   /**
    * Destructor
    */
   ~MessageRequestHandler();
   
   /**
    *
    */
   virtual void run();
   
private:
   MessageHandler* m_handler;
};

}

#endif
