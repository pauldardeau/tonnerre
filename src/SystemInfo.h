// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef __HttpServer__SystemInfo__
#define __HttpServer__SystemInfo__

#include <string>


/*!
 * Wrapper/utility class around uname function
 */
class SystemInfo
{
public:
   /**
    * Default constructor and retrieves the system information
    */
   SystemInfo() noexcept;
   
   /**
    * Copy constructor
    * @param copy the source of the copy
    */
   SystemInfo(const SystemInfo& copy) noexcept;
   
   /**
    * Move constructor
    * @param move the source of the move
    */
   SystemInfo(SystemInfo&& move) noexcept;
   
   /**
    * Destructor
    */
   ~SystemInfo() noexcept;
   
   /**
    * Copy operator
    * @param copy the source of the copy
    * @return reference to the updated instance
    */
   SystemInfo& operator=(const SystemInfo& copy) noexcept;
   
   /**
    * Move operator
    * @param move the source of the move
    * @return reference to the target of the move
    */
   SystemInfo& operator=(SystemInfo&& move) noexcept;
   
   /**
    * Retrieves the system name (sysname)
    * @return the system name
    */
   const std::string& sysName() const noexcept;
   
   /**
    * Retrieves the node name (nodename)
    * @return the node name
    */
   const std::string& nodeName() const noexcept;
   
   /**
    * Retrieves the release (release)
    * @return the release
    */
   const std::string& release() const noexcept;
   
   /**
    * Retrieves the version (version)
    * @return the version
    */
   const std::string& version() const noexcept;
   
   /**
    * Retrieves the machine (machine)
    * @return the machine
    */
   const std::string& machine() const noexcept;
   
   /**
    * Determines if the system information was able to be retrieved
    * @return boolean indicating whether the system information was successfully read
    */
   bool retrievedSystemInfo() const noexcept;

   
private:
   std::string m_sysName;
   std::string m_nodeName;
   std::string m_release;
   std::string m_version;
   std::string m_machine;
   bool m_retrievedSystemInfo;
   
};

#endif /* defined(__HttpServer__SystemInfo__) */
