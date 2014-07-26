// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef REQUESTHANDLER_H
#define REQUESTHANDLER_H


#include "Runnable.h"

class Socket;
class SocketRequest;


/**
 *
 */
class RequestHandler : public Runnable
{
private:
   std::shared_ptr<Socket> m_socket;
   std::shared_ptr<SocketRequest> m_socketRequest;
   bool m_isThreadPooling;
   
public:
   /**
    * Constructs a RequestHandler using a SocketRequest for use by a KernelEventServer
    * @param socketRequest the SocketRequest for processing using KernelEventServer
    * @see SocketRequest()
    */
   RequestHandler(std::shared_ptr<SocketRequest> socketRequest) noexcept;
   
   /**
    * Constructs a RequestHandler using a Socket
    * @param socket the Socket for handling the request
    * @see Socket()
    */
   RequestHandler(std::shared_ptr<Socket> socket) noexcept;
   
   /**
    * Destructor
    */
   ~RequestHandler() noexcept;
   
   /**
    * Sets boolean indicating whether request is being run on thread pool
    * @param isThreadPooling boolean indicating if request is being run on thread pool
    */
   void setThreadPooling(bool isThreadPooling) noexcept;
   
   /**
    *
    * @return
    * @see Socket()
    */
   std::shared_ptr<Socket> getSocket() noexcept;
   
   
   
   // disallow copies
   RequestHandler(const RequestHandler&) = delete;
   RequestHandler(RequestHandler&&) = delete;
   RequestHandler& operator=(const RequestHandler&) = delete;
   RequestHandler& operator=(RequestHandler&&) = delete;
   
};

#endif
