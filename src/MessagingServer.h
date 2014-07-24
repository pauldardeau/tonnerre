#ifndef MESSAGINGSERVER_H
#define MESSAGINGSERVER_H

#include <string>

#include "SocketServer.h"

class MessageHandler;


class MessagingServer : public SocketServer
{
public:
   MessagingServer(const std::string& configFilePath,
                   const std::string& serviceName,
                   MessageHandler* handler);
   ~MessagingServer();

   virtual std::shared_ptr<RequestHandler> handlerForSocket(std::shared_ptr<Socket> socket);
   virtual std::shared_ptr<RequestHandler> handlerForSocketRequest(std::shared_ptr<SocketRequest> socketRequest);

   
private:
   MessageHandler* m_handler;
   std::string m_serviceName;
};

#endif
