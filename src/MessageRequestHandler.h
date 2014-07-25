// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef MESSAGEREQUESTHANDLER_H
#define MESSAGEREQUESTHANDLER_H

#include "RequestHandler.h"

class MessageHandler;
class Socket;
class SocketRequest;


class MessageRequestHandler : public RequestHandler
{
public:
   MessageRequestHandler(std::shared_ptr<Socket> socket, MessageHandler* handler);
   MessageRequestHandler(std::shared_ptr<SocketRequest> socketRequest, MessageHandler* handler);
   ~MessageRequestHandler();
   
   virtual void run();
   
private:
   MessageHandler* m_handler;
};

#endif
