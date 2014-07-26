// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef MESSAGEREQUESTHANDLER_H
#define MESSAGEREQUESTHANDLER_H

#include "RequestHandler.h"

class MessageHandler;
class Socket;
class SocketRequest;


/**
 *
 */
class MessageRequestHandler : public RequestHandler
{
public:
   /**
    *
    * @param socket
    * @param handler
    * @see Socket()
    * @see MessageHandler()
    */
   MessageRequestHandler(std::shared_ptr<Socket> socket, MessageHandler* handler);
   
   /**
    *
    * @param socketRequest
    * @param handler
    * @see SocketRequest()
    * @see MessageHandler()
    */
   MessageRequestHandler(std::shared_ptr<SocketRequest> socketRequest, MessageHandler* handler);
   
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

#endif
