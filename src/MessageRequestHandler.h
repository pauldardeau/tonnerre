#ifndef MESSAGEREQUESTHANDLER_H
#define MESSAGEREQUESTHANDLER_H

#include "RequestHandler.h"

class Socket;
class SocketRequest;


class MessageRequestHandler : public RequestHandler
{
public:
   MessageRequestHandler(std::shared_ptr<Socket> socket);
   MessageRequestHandler(std::shared_ptr<SocketRequest> socketRequest);
   ~MessageRequestHandler();
   
   virtual void run();
   
};

#endif
