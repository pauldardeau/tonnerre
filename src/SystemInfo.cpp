// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#include <sys/utsname.h>

#include "SystemInfo.h"
#include "Logger.h"

//******************************************************************************

SystemInfo::SystemInfo() noexcept :
   m_retrievedSystemInfo(false)
{
   Logger::logInstanceCreate("SystemInfo");

   struct utsname sysinfo;
   if (0 == ::uname(&sysinfo)) {
      m_sysName = sysinfo.sysname;
      m_nodeName = sysinfo.nodename;
      m_release = sysinfo.release;
      m_version = sysinfo.version;
      m_machine = sysinfo.machine;
      m_retrievedSystemInfo = true;
   }
}

//******************************************************************************

SystemInfo::SystemInfo(const SystemInfo& copy) noexcept :
   m_sysName(copy.m_sysName),
   m_nodeName(copy.m_nodeName),
   m_release(copy.m_release),
   m_version(copy.m_version),
   m_machine(copy.m_machine),
   m_retrievedSystemInfo(copy.m_retrievedSystemInfo)
{
   Logger::logInstanceCreate("SystemInfo");
}

//******************************************************************************

SystemInfo::SystemInfo(SystemInfo&& move) noexcept :
   m_sysName(std::move(move.m_sysName)),
   m_nodeName(std::move(move.m_nodeName)),
   m_release(std::move(move.m_release)),
   m_version(std::move(move.m_version)),
   m_machine(std::move(move.m_machine)),
   m_retrievedSystemInfo(move.m_retrievedSystemInfo)
{
   Logger::logInstanceCreate("SystemInfo");

   move.m_retrievedSystemInfo = false;
}

//******************************************************************************

SystemInfo::~SystemInfo() noexcept
{
   Logger::logInstanceDestroy("SystemInfo");
}

//******************************************************************************

SystemInfo& SystemInfo::operator=(const SystemInfo& copy) noexcept
{
   if (this ==&copy) {
      return *this;
   }
   
   m_sysName = copy.m_sysName;
   m_nodeName = copy.m_nodeName;
   m_release = copy.m_release;
   m_version = copy.m_version;
   m_machine = copy.m_machine;
   m_retrievedSystemInfo = copy.m_retrievedSystemInfo;
   
   return *this;
}

//******************************************************************************

SystemInfo& SystemInfo::operator=(SystemInfo&& move) noexcept
{
   if (this == &move) {
      return *this;
   }
   
   m_sysName = std::move(move.m_sysName);
   m_nodeName = std::move(move.m_nodeName);
   m_release = std::move(move.m_release);
   m_version = std::move(move.m_version);
   m_machine = std::move(m_machine);
   m_retrievedSystemInfo = move.m_retrievedSystemInfo;
   
   move.m_retrievedSystemInfo = false;
   
   return *this;
}

//******************************************************************************

const std::string& SystemInfo::sysName() const noexcept
{
   return m_sysName;
}

//******************************************************************************

const std::string& SystemInfo::nodeName() const noexcept
{
   return m_nodeName;
}

//******************************************************************************

const std::string& SystemInfo::release() const noexcept
{
   return m_release;
}

//******************************************************************************

const std::string& SystemInfo::version() const noexcept
{
   return m_version;
}

//******************************************************************************

const std::string& SystemInfo::machine() const noexcept
{
   return m_machine;
}

//******************************************************************************

bool SystemInfo::retrievedSystemInfo() const noexcept
{
   return m_retrievedSystemInfo;
}

//******************************************************************************

