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
#include "PthreadsThreadingFactory.h"
#include "StdThreadingFactory.h"

using namespace tonnerre;
using namespace chaudiere;

static const std::string KEY_HOST        = "host";
static const std::string KEY_PERSISTENT  = "persistent";
static const std::string KEY_PORT        = "port";
static const std::string KEY_SERVICES    = "services";

static const std::string VALUE_TRUE      = "true";


Messaging* Messaging::messagingInstance = NULL;

//******************************************************************************

void Messaging::setMessaging(Messaging* messaging) noexcept {
   messagingInstance = messaging;
}

//******************************************************************************

Messaging* Messaging::getMessaging() noexcept {
   return messagingInstance;
}

//******************************************************************************

// throws BasicException
void Messaging::initialize(const std::string& configFilePath) {
   IniReader reader(configFilePath);
   if (reader.hasSection(KEY_SERVICES)) {
      KeyValuePairs kvpServices;
      if (reader.readSection(KEY_SERVICES, kvpServices)) {
         std::vector<std::string> keys;
         kvpServices.getKeys(keys);
         int servicesRegistered = 0;
         
         Messaging* messaging(new Messaging);
         
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

bool Messaging::isInitialized() {
   return NULL != getMessaging();
}

//******************************************************************************

Messaging::Messaging() :
   m_mutex(NULL) {
   ThreadingFactory* factory = ThreadingFactory::getThreadingFactory();
   if (factory == NULL) {
      //factory = new PthreadsThreadingFactory;
      factory = new StdThreadingFactory;
      ThreadingFactory::setThreadingFactory(factory);
   }
  
   m_mutex = factory->createMutex("messaging");
}

//******************************************************************************

Messaging::~Messaging() {
}

//******************************************************************************

void Messaging::registerService(const std::string& serviceName,
                                const ServiceInfo& serviceInfo) {
   m_mapServices[serviceName] = serviceInfo;
}

//******************************************************************************

bool Messaging::isServiceRegistered(const std::string& serviceName) const {
   return (m_mapServices.find(serviceName) != m_mapServices.end());
}

//******************************************************************************

const ServiceInfo& Messaging::getInfoForService(const std::string& serviceName) const {
   auto it = m_mapServices.find(serviceName);
   if (it != m_mapServices.end()) {
      return (*it).second;
   } else {
      throw InvalidKeyException(serviceName);
      return (*it).second;
   }
}

//******************************************************************************

Socket* Messaging::socketForService(const ServiceInfo& serviceInfo) {
   const std::string serviceId = serviceInfo.getUniqueIdentifier();
   printf("serviceId=%s\n", serviceId.c_str());
   MutexLock lock(*m_mutex);
   auto it = m_mapSocketConnections.find(serviceId);
   if (it != m_mapSocketConnections.end()) {
      printf("using existing socket\n");
      Socket* socket = (*it).second;
      m_mapSocketConnections.erase(it);
      return socket;
   } else {
      std::string host = serviceInfo.host();
      int port = serviceInfo.port();
      printf("creating new socket, host=%s, port=%d\n", host.c_str(), port);
      return new Socket(serviceInfo.host(),
                        serviceInfo.port());
   }
}

//******************************************************************************

void Messaging::returnSocketForService(const ServiceInfo& serviceInfo,
                                       Socket* socket) {
   const std::string serviceId = serviceInfo.getUniqueIdentifier();
   MutexLock lock(*m_mutex);
   m_mapSocketConnections[serviceId] = socket;
}

//******************************************************************************
