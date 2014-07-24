// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef C10KServer_SocketServiceHandler_h
#define C10KServer_SocketServiceHandler_h

#include <string>

class SocketRequest;

/**
 * SocketServiceHandler is an interface for a handler the processes a SocketRequest
 */
class SocketServiceHandler
{
public:
   /**
    * Destructor
    */
   virtual ~SocketServiceHandler() {}
   
   /**
    * Process a SocketRequest
    * @param socketRequest the SocketRequest to process
    * @see SocketRequest()
    */
   virtual void serviceSocket(std::shared_ptr<SocketRequest> socketRequest) = 0;
   
   /**
    * Retrieves the name of the handler. This is primarily an aid for debugging.
    * @return the name of the handler
    */
   virtual const std::string& getName() const = 0;
};


#endif
