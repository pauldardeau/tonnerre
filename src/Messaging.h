// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef TONNERRE_MESSAGING_H
#define TONNERRE_MESSAGING_H

#include <string>
#include <unordered_map>

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
   static void setMessaging(std::shared_ptr<Messaging> messaging) noexcept;
   
   /**
    * Retrieves the Messaging singleton instance
    * @return pointer to the Messaging instance, or nullptr if not initialized
    */
   static std::shared_ptr<Messaging> getMessaging() noexcept;
   
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
   void registerService(const std::string& serviceName, const chaudiere::ServiceInfo& serviceInfo);
   
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
   std::shared_ptr<chaudiere::Socket> socketForService(const chaudiere::ServiceInfo& serviceInfo);

   /**
    * Returns socket connection back to pool for reuse
    * @param serviceInfo the service whose socket connection is being returned to pool
    * @param socket the socket connection being returned to the pool
    * @see ServiceInfo()
    * @see Socket()
    */
   void returnSocketForService(const chaudiere::ServiceInfo& serviceInfo,
                               std::shared_ptr<chaudiere::Socket> socket);


   Messaging(const Messaging&) = delete;
   Messaging(Messaging&&) = delete;
   Messaging& operator=(const Messaging&) = delete;
   Messaging& operator=(Messaging&&) = delete;

private:
   static std::shared_ptr<Messaging> messagingInstance;
   std::unordered_map<std::string, chaudiere::ServiceInfo> m_mapServices;
   std::unordered_map<std::string, std::shared_ptr<chaudiere::Socket>> m_mapSocketConnections;
   std::shared_ptr<chaudiere::Mutex> m_mutex;
};

}

#endif
