// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef MESSAGING_H
#define MESSAGING_H

#include <string>
#include <unordered_map>

#include "ServiceInfo.h"


class Messaging
{
public:
   static void setMessaging(std::shared_ptr<Messaging> messaging) noexcept;
   static std::shared_ptr<Messaging> getMessaging() noexcept;
   static void initialize(const std::string& configFilePath);
   static bool isInitialized();
   
   void registerService(const std::string& serviceName, const ServiceInfo& serviceInfo);
   bool isServiceRegistered(const std::string& serviceName) const;
   const ServiceInfo& getInfoForService(const std::string& serviceName) const;

private:
   static std::shared_ptr<Messaging> messagingInstance;
   std::unordered_map<std::string, ServiceInfo> m_mapServices;
};

#endif
