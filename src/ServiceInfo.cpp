// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#include "ServiceInfo.h"

//******************************************************************************

ServiceInfo::ServiceInfo() :
   m_port(0)
{
}

//******************************************************************************

ServiceInfo::ServiceInfo(const std::string& serviceName, const std::string& host, unsigned short port) :
   m_serviceName(serviceName),
   m_host(host),
   m_port(port)
{
}

//******************************************************************************

ServiceInfo::ServiceInfo(const ServiceInfo& copy) :
   m_serviceName(copy.m_serviceName),
   m_host(copy.m_host),
   m_port(copy.m_port)
{
}

//******************************************************************************

ServiceInfo::ServiceInfo(ServiceInfo&& move) :
   m_serviceName(std::move(move.m_serviceName)),
   m_host(std::move(move.m_host)),
   m_port(move.m_port)
{
}

//******************************************************************************

ServiceInfo::~ServiceInfo()
{
}

//******************************************************************************
   
ServiceInfo& ServiceInfo::operator=(const ServiceInfo& copy)
{
   if (this == &copy) {
      return *this;
   }
   
   m_serviceName = copy.m_serviceName;
   m_host = copy.m_host;
   m_port = copy.m_port;
   
   return *this;
}

//******************************************************************************

ServiceInfo& ServiceInfo::operator=(ServiceInfo&& move)
{
   if (this == &move) {
      return *this;
   }
   
   m_serviceName = std::move(move.m_serviceName);
   m_host = std::move(move.m_host);
   m_port = move.m_port;
   
   return *this;
}

//******************************************************************************
   
const std::string& ServiceInfo::serviceName() const
{
   return m_serviceName;
}

//******************************************************************************

const std::string& ServiceInfo::host() const
{
   return m_host;
}

//******************************************************************************

unsigned short ServiceInfo::port() const
{
   return m_port;
}
   
//******************************************************************************
