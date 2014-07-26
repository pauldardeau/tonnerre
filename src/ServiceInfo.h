// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef SERVICEINFO_H
#define SERVICEINFO_H

#include <string>


/**
 *
 */
class ServiceInfo
{
public:
   /**
    *
    */
   ServiceInfo();
   
   /**
    *
    * @param serviceName
    * @param host
    * @param port
    */
   ServiceInfo(const std::string& serviceName, const std::string& host, unsigned short port);
   
   /**
    * Copy constructor
    * @param copy the source of the copy
    */
   ServiceInfo(const ServiceInfo& copy);
   
   /**
    * Move constructor
    * @param move the source of the move
    */
   ServiceInfo(ServiceInfo&& move);
   
   /**
    * Destructor
    */
   ~ServiceInfo();
   
   /**
    * Copy operator
    * @param copy the source of the copy
    * @return reference to the target of the copy
    */
   ServiceInfo& operator=(const ServiceInfo& copy);
   
   /**
    * Move operator
    * @param move the source of the move
    * @return reference to the target of the move
    */
   ServiceInfo& operator=(ServiceInfo&& move);
   
   /**
    * Retrieves the name of the sevice
    * @return the service name
    */
   const std::string& serviceName() const;
   
   /**
    * Retrieves the host for the service
    * @return the service host
    */
   const std::string& host() const;
   
   /**
    * Retrieves the port number for the service
    * @return the service port number
    */
   unsigned short port() const;
   
private:
   std::string m_serviceName;
   std::string m_host;
   unsigned short m_port;
};

#endif
