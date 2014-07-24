#ifndef MESSAGEREQUESTHANDLER_H
#define MESSAGEREQUESTHANDLER_H

#include "RequestHandler.h"

class Socket;
class SocketRequest;
class SocketServer;


class MessageRequestHandler : public RequestHandler
{
public:
   MessageRequestHandler(SocketServer& socketServer, std::shared_ptr<Socket> socket);
   MessageRequestHandler(SocketServer& socketServer, std::shared_ptr<SocketRequest> socketRequest);
   
   ~MessageRequestHandler();
   
   virtual void run();
   
private:
   SocketServer& m_socketServer;
};

#endif
