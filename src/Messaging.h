// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef MESSAGING_H
#define MESSAGING_H

#include <string>
#include <unordered_map>

#include "ServiceInfo.h"

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
    * Registers a service with its name and host/port values
    * @param serviceName the name of the sevice being registered
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

private:
   static std::shared_ptr<Messaging> messagingInstance;
   std::unordered_map<std::string, chaudiere::ServiceInfo> m_mapServices;
};

}

#endif
