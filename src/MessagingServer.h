// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef TONNERRE_MESSAGINGSERVER_H
#define TONNERRE_MESSAGINGSERVER_H

#include <string>

#include "SocketServer.h"
#include "RequestHandler.h"
#include "SocketServiceHandler.h"


namespace tonnerre
{
   class MessageHandler;

/**
 *
 */
class MessagingServer : public chaudiere::SocketServer
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
                   MessageHandler* handler=nullptr);
                   
   /**
    * Destructor
    */
   ~MessagingServer();
   
   /**
    * Sets the message handler
    * @param handler the handler to use
    */
   void setMessageHandler(MessageHandler* handler);

   /**
    * Creates a request handler for the socket (used internally)
    * @param socket the socket that will be used by the new handler
    * @return a new instance of RequestHandler to service the socket
    * @see RequestHandler()
    * @see Socket()
    */
   virtual chaudiere::RequestHandler* handlerForSocket(chaudiere::Socket* socket);
   
  /**
   * Creates a request handler for the SocketRequest (used internally)
   * @param socketRequest the socket request that will be used by the new handler
   * @return a new instance of RequestHandler to service the socket
   * @see RequestHandler()
   * @see SocketRequest()
   */
   virtual chaudiere::RequestHandler* handlerForSocketRequest(chaudiere::SocketRequest* socketRequest);

  /**
   * Creates a socket service handler (used internally)
   * @return a new instance of SocketServiceHandler
   * @see SocketServiceHandler()
   */
   virtual chaudiere::SocketServiceHandler* createSocketServiceHandler();
   
private:
   MessageHandler* m_handler;
   std::string m_serviceName;
};

}

#endif
