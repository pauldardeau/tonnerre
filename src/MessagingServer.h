// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef MESSAGINGSERVER_H
#define MESSAGINGSERVER_H

#include <string>

#include "SocketServer.h"

class MessageHandler;
class RequestHandler;
class SocketServiceHandler;

/**
 *
 */
class MessagingServer : public SocketServer
{
public:
   static const std::string SERVER_NAME;
   static const std::string SERVER_VERSION;
   
   /**
    * Constructs a new server to receive messages
    * @param configFilePath the path to the configuration (INI) file
    * @param serviceName the name of the service this server will provide (as defined in configuration file)
    * @param handler the handler to be called when message requests are received
    */
   MessagingServer(const std::string& configFilePath,
                   const std::string& serviceName,
                   MessageHandler* handler);
                   
   /**
    * Destructor
    */
   ~MessagingServer();

   /**
    * Creates a request handler for the socket (used internally)
    * @param socket the socket that will be used by the new handler
    * @return a new instance of RequestHandler to service the socket
    * @see RequestHandler()
    * @see Socket()
    */
   virtual std::shared_ptr<RequestHandler> handlerForSocket(std::shared_ptr<Socket> socket);
   
  /**
   * Creates a request handler for the SocketRequest (used internally)
   * @param socketRequest the socket request that will be used by the new handler
   * @return a new instance of RequestHandler to service the socket
   * @see RequestHandler()
   * @see SocketRequest()
   */
   virtual std::shared_ptr<RequestHandler> handlerForSocketRequest(std::shared_ptr<SocketRequest> socketRequest);

  /**
   * Creates a socket service handler (used internally)
   * @return a new instance of SocketServiceHandler
   * @see SocketServiceHandler()
   */
   virtual std::shared_ptr<SocketServiceHandler> createSocketServiceHandler();
   
private:
   MessageHandler* m_handler;
   std::string m_serviceName;
};

#endif
