// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef TONNERRE_MESSAGING_H
#define TONNERRE_MESSAGING_H

#include <string>
#include <map>

#include "ServiceInfo.h"
#include "Socket.h"
#include "Mutex.h"


namespace tonnerre
{

/**
 * Messaging class provides a global entry point for initializing the messaging system
 */
class Messaging
{
public:
   /**
    * Establishes a Messaging instance as the singleton for messaging
    * @param messaging the Messaging object instance for messaging
    */
   static void setMessaging(Messaging* messaging);
   
   /**
    * Retrieves the Messaging singleton instance
    * @return pointer to the Messaging instance, or NULL if not initialized
    */
   static Messaging* getMessaging();
   
   /**
    * Initializes the messaging system by reading the configuration file and creating a Messaging instance
    * @param configFilePath the file path to the INI configuration file
    */
   static void initialize(const std::string& configFilePath);
   
   /**
    * Determines if the messaging system has been initialized
    * @return boolean indicating if messaging system has been initialized
    */
   static bool isInitialized();
   
   /**
    * Default constructor
    */
   Messaging();
   
   /**
    * Destructor
    */
   ~Messaging();
   
   /**
    * Registers a service with its name and host/port values
    * @param serviceName the name of the service being registered
    * @param serviceInfo the host/port values for the service
    * @see ServiceInfo()
    */
   void registerService(const std::string& serviceName,
                        const chaudiere::ServiceInfo& serviceInfo);
   
   /**
    * Determines if the specified service name has been registered
    * @param serviceName the service name whose existence is being evaluated
    * @return boolean indicating if the service has been registered
    */
   bool isServiceRegistered(const std::string& serviceName) const;
   
   /**
    * Retrieves the host and port values for the specified service name
    * @param serviceName the name of the service whose host/port values are being requested
    * @return object holding the host/port values for the service
    * @see ServiceInfo()
    */
   const chaudiere::ServiceInfo& getInfoForService(const std::string& serviceName) const;
   
   /**
    * Retrieve a socket connection for the specified service
    * @param serviceInfo the service for which a socket conection is desired
    * @return socket connection
    * @see ServiceInfo()
    * @see Socket()
    */
   chaudiere::Socket* socketForService(const chaudiere::ServiceInfo& serviceInfo);

   /**
    * Returns socket connection back to pool for reuse
    * @param serviceInfo the service whose socket connection is being returned to pool
    * @param socket the socket connection being returned to the pool
    * @see ServiceInfo()
    * @see Socket()
    */
   void returnSocketForService(const chaudiere::ServiceInfo& serviceInfo,
                               chaudiere::Socket* socket);



private:
   static Messaging* messagingInstance;
   std::map<std::string, chaudiere::ServiceInfo> m_mapServices;
   std::map<std::string, chaudiere::Socket*> m_mapSocketConnections;
   chaudiere::Mutex* m_mutex;
   
   Messaging(const Messaging&);
   Messaging& operator=(const Messaging&);
};

}

#endif
