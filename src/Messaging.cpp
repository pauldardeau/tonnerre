// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#include <vector>
#include <stdlib.h>

#include "Messaging.h"
#include "IniReader.h"
#include "InvalidKeyException.h"
#include "KeyValuePairs.h"
#include "MutexLock.h"
#include "ThreadingFactory.h"

using namespace tonnerre;
using namespace chaudiere;

static const std::string KEY_HOST        = "host";
static const std::string KEY_PERSISTENT  = "persistent";
static const std::string KEY_PORT        = "port";
static const std::string KEY_SERVICES    = "services";

static const std::string VALUE_TRUE      = "true";


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
   if (reader.hasSection(KEY_SERVICES)) {
      KeyValuePairs kvpServices;
      if (reader.readSection(KEY_SERVICES, kvpServices)) {
         std::vector<std::string> keys;
         kvpServices.getKeys(keys);
         int servicesRegistered = 0;
         
         std::shared_ptr<Messaging> messaging(new Messaging);
         
         for (auto key : keys) {
            const std::string& serviceName = key;
            const std::string& sectionName = kvpServices.getValue(serviceName);
            
            KeyValuePairs kvp;
            if (reader.readSection(sectionName, kvp)) {
               if (kvp.hasKey(KEY_HOST) && kvp.hasKey(KEY_PORT)) {
                  const std::string& host = kvp.getValue(KEY_HOST);
                  const std::string& portAsString = kvp.getValue(KEY_PORT);
                  const unsigned short portValue =
                     (unsigned short) ::atoi(portAsString.c_str());
                  
                  ServiceInfo serviceInfo(serviceName, host, portValue);
                  
                  if (kvp.hasKey(KEY_PERSISTENT)) {
                     const std::string& persistence =
                        kvp.getValue(KEY_PERSISTENT);
                     if (persistence == VALUE_TRUE) {
                        serviceInfo.setPersistentConnection(true);
                     }
                  }
                  
                  messaging->registerService(serviceName, serviceInfo);
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

Messaging::Messaging() :
   m_mutex(nullptr)
{
   std::shared_ptr<ThreadingFactory> factory(ThreadingFactory::getThreadingFactory());
   if (factory == nullptr) {
      factory = std::shared_ptr<ThreadingFactory>(new ThreadingFactory(ThreadingFactory::ThreadingPackage::PTHREADS));
      ThreadingFactory::setThreadingFactory(factory);
   }
   
   m_mutex = std::shared_ptr<Mutex>(factory->createMutex());
}

//******************************************************************************

Messaging::~Messaging()
{
}

//******************************************************************************

void Messaging::registerService(const std::string& serviceName,
                                const ServiceInfo& serviceInfo)
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

std::shared_ptr<Socket> Messaging::socketForService(const ServiceInfo& serviceInfo)
{
   const std::string serviceId = serviceInfo.getUniqueIdentifier();
   MutexLock lock(*m_mutex);
   auto it = m_mapSocketConnections.find(serviceId);
   if (it != m_mapSocketConnections.end()) {
      std::shared_ptr<Socket> socket = (*it).second;
      m_mapSocketConnections.erase(it);
      return socket;
   } else {
      return std::unique_ptr<Socket>(new Socket(serviceInfo.host(),
                                                serviceInfo.port()));
   }
}

//******************************************************************************

void Messaging::returnSocketForService(const ServiceInfo& serviceInfo,
                                       std::shared_ptr<Socket> socket)
{
   const std::string serviceId = serviceInfo.getUniqueIdentifier();
   MutexLock lock(*m_mutex);
   m_mapSocketConnections[serviceId] = std::move(socket);
}

//******************************************************************************
