// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License
#include <vector>
#include <stdlib.h>

#include "Messaging.h"
#include "IniReader.h"
#include "InvalidKeyException.h"
#include "KeyValuePairs.h"

std::shared_ptr<Messaging> Messaging::messagingInstance = nullptr;

//******************************************************************************

void Messaging::setMessaging(std::shared_ptr<Messaging> messaging) noexcept
{
   messagingInstance = messaging;
}

//******************************************************************************

std::shared_ptr<Messaging> Messaging::getMessaging() noexcept
{
   return messagingInstance;
}

//******************************************************************************

// throws BasicException
void Messaging::initialize(const std::string& configFilePath)
{
   IniReader reader(configFilePath);
   if (reader.hasSection("services")) {
      KeyValuePairs kvpServices;
      if (reader.readSection("services", kvpServices)) {
         std::vector<std::string> keys;
         kvpServices.getKeys(keys);
         int servicesRegistered = 0;
         
         std::shared_ptr<Messaging> messaging(new Messaging);
         
         for (auto key : keys) {
            const std::string& serviceName = key;
            const std::string& sectionName = kvpServices.getValue(serviceName);
            
            KeyValuePairs kvp;
            if (reader.readSection(sectionName, kvp)) {
               if (kvp.hasKey("host") && kvp.hasKey("port")) {
                  const std::string& host = kvp.getValue("host");
                  const std::string& portAsString = kvp.getValue("port");
                  const unsigned short portValue = (unsigned short) ::atoi(portAsString.c_str());
                  
                  ServiceInfo serviceInfo(serviceName, host, portValue);
                  messaging->registerService(serviceName, serviceInfo);
                  //messaging->m_mapServices[serviceName] = std::move(serviceInfo);
                  ++servicesRegistered;
               }
            }
         }
         
         if (servicesRegistered > 0) {
            Messaging::setMessaging(messaging);
         }
      }
   }
}

//******************************************************************************

bool Messaging::isInitialized()
{
   return nullptr != getMessaging();
}

//******************************************************************************

void Messaging::registerService(const std::string& serviceName, const ServiceInfo& serviceInfo)
{
   m_mapServices[serviceName] = serviceInfo;
}

//******************************************************************************

bool Messaging::isServiceRegistered(const std::string& serviceName) const
{
   return (m_mapServices.find(serviceName) != m_mapServices.end());
}

//******************************************************************************

const ServiceInfo& Messaging::getInfoForService(const std::string& serviceName) const
{
   auto it = m_mapServices.find(serviceName);
   if (it != m_mapServices.end()) {
      return (*it).second;
   } else {
      throw InvalidKeyException(serviceName);
      return (*it).second;
   }
}

//******************************************************************************
