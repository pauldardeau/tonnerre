// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef SERVICEINFO_H
#define SERVICEINFO_H

#include <string>


class ServiceInfo
{
public:
   ServiceInfo();
   ServiceInfo(const std::string& serviceName, const std::string& host, unsigned short port);
   ServiceInfo(const ServiceInfo& copy);
   ServiceInfo(ServiceInfo&& move);
   ~ServiceInfo();
   
   ServiceInfo& operator=(const ServiceInfo& copy);
   ServiceInfo& operator=(ServiceInfo&& move);
   
   const std::string& serviceName() const;
   const std::string& host() const;
   unsigned short port() const;
   
private:
   std::string m_serviceName;
   std::string m_host;
   unsigned short m_port;
};

#endif
