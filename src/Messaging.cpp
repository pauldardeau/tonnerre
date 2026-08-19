// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#include <vector>
#include <stdlib.h>

#include "Messaging.h"
#include "IniReader.h"
#include "InvalidKeyException.h"
#include "KeyValuePairs.h"
#include "MutexLock.h"
#include "PthreadsThreadingFactory.h"
#include "StrUtils.h"

using namespace std;
using namespace tonnerre;
using namespace chaudiere;

static const std::string KEY_HOST        = "host";
static const std::string KEY_PERSISTENT  = "persistent";
static const std::string KEY_PORT        = "port";
static const std::string KEY_SERVICES    = "services";

static const std::string VALUE_TRUE      = "true";
static const std::string EMPTY           = "";


std::shared_ptr<Messaging> Messaging::messagingInstance;

//******************************************************************************

// messagingInstance is accessed via std::atomic_load/atomic_store rather
// than a plain pointer, so concurrent setMessaging()/getMessaging() calls
// from different threads don't race on the pointer itself (the same
// pattern used for chaudiere::Logger::loggerInstance). std::atomic_store
// also means a prior instance is now actually released when replaced,
// rather than leaked as it was with the raw pointer.

void Messaging::setMessaging(Messaging* messaging)
{
   std::atomic_store(&messagingInstance, std::shared_ptr<Messaging>(messaging));
}

//******************************************************************************

std::shared_ptr<Messaging> Messaging::getMessaging()
{
   return std::atomic_load(&messagingInstance);
}

//******************************************************************************

// throws BasicException
void Messaging::initialize(const std::string& configFilePath)
{
   IniReader reader(configFilePath);
   if (reader.hasSection(KEY_SERVICES)) {
      KeyValuePairs kvpServices;
      if (reader.readSection(KEY_SERVICES, kvpServices)) {
         vector<string> keys;
         kvpServices.getKeys(keys);
         int servicesRegistered = 0;

         Messaging* messaging(new Messaging);

         for (const auto& serviceName : keys) {
            const string& sectionName = kvpServices.getValue(serviceName);

            KeyValuePairs kvp;
            if (reader.readSection(sectionName, kvp)) {
               if (kvp.hasKey(KEY_HOST) && kvp.hasKey(KEY_PORT)) {
                  const string& host = kvp.getValue(KEY_HOST);
                  const string& portAsString = kvp.getValue(KEY_PORT);
                  const unsigned short portValue =
                     (unsigned short) StrUtils::parseInt(portAsString);

                  ServiceInfo serviceInfo(serviceName, host, portValue);

                  if (kvp.hasKey(KEY_PERSISTENT)) {
                     const string& persistence =
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
   ThreadingFactory* factory = ThreadingFactory::getThreadingFactory();
   if (factory == nullptr) {
      factory = new PthreadsThreadingFactory();
      ThreadingFactory::setThreadingFactory(factory);
   }

   m_mutex.reset(factory->createMutex(EMPTY));
}

//******************************************************************************

Messaging::~Messaging()
{
}

//******************************************************************************

void Messaging::registerService(const std::string& serviceName,
                                const ServiceInfo& serviceInfo)
{
   MutexLock lock(*m_mutex);
   m_mapServices[serviceName] = serviceInfo;
}

//******************************************************************************

bool Messaging::isServiceRegistered(const std::string& serviceName) const
{
   MutexLock lock(*m_mutex);
   return (m_mapServices.find(serviceName) != m_mapServices.end());
}

//******************************************************************************

ServiceInfo Messaging::getInfoForService(const std::string& serviceName) const
{
   MutexLock lock(*m_mutex);
   const map<string,ServiceInfo>::const_iterator it =
      m_mapServices.find(serviceName);
   if (it != m_mapServices.end()) {
      return (*it).second;
   } else {
      throw InvalidKeyException(serviceName);
   }
}

//******************************************************************************

Socket* Messaging::socketForService(const ServiceInfo& serviceInfo)
{
   const string serviceId = serviceInfo.getUniqueIdentifier();
   MutexLock lock(*m_mutex);
   map<string,Socket*>::iterator it =
      m_mapSocketConnections.find(serviceId);
   if (it != m_mapSocketConnections.end()) {
      Socket* socket = (*it).second;
      m_mapSocketConnections.erase(it);
      return socket;
   } else {
      return new Socket(serviceInfo.host(),
                        serviceInfo.port());
   }
}

//******************************************************************************

void Messaging::returnSocketForService(const ServiceInfo& serviceInfo,
                                       Socket* socket)
{
   const string serviceId = serviceInfo.getUniqueIdentifier();
   MutexLock lock(*m_mutex);
   m_mapSocketConnections[serviceId] = socket;
}

//******************************************************************************
