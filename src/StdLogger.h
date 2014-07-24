// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef __HttpServer__StdLogger__
#define __HttpServer__StdLogger__

#include <string>
#include <unordered_map>
#include <memory>

#include "Logger.h"
#include "Mutex.h"


class LifecycleStats
{
public:
   long long m_instancesCreated;
   long long m_instancesDestroyed;
   
   LifecycleStats() :
      m_instancesCreated(0L),
      m_instancesDestroyed(0L)
   {
   }
   
   LifecycleStats(const LifecycleStats& copy) :
      m_instancesCreated(copy.m_instancesCreated),
      m_instancesDestroyed(copy.m_instancesDestroyed)
   {
   }
   
   LifecycleStats& operator=(const LifecycleStats& copy)
   {
      if (this == &copy) {
         return *this;
      }
      
      m_instancesCreated = copy.m_instancesCreated;
      m_instancesDestroyed = copy.m_instancesDestroyed;
      
      return *this;
   }
};

/*!
 * StdLogger is the default logger and logs to the console.
 */
class StdLogger : public Logger
{
public:
   StdLogger() noexcept;
   StdLogger(LogLevel logLevel) noexcept;
   virtual ~StdLogger();
   
   virtual LogLevel getLogLevel() const noexcept override;
   virtual void setLogLevel(LogLevel logLevel) noexcept override;

   virtual void logMessage(LogLevel logLevel, const std::string& logMessage) noexcept override;
   virtual bool isLoggingLevel(LogLevel logLevel) const noexcept override;
   
   virtual bool isLoggingInstanceLifecycles() const noexcept override;
   virtual void setLogInstanceLifecycles(bool logInstanceLifecycles) noexcept override;
   virtual void logInstanceCreate(const std::string& className) noexcept override;
   virtual void logInstanceDestroy(const std::string& className) noexcept override;
   
   virtual void logOccurrence(const std::string& occurrenceType,
                              const std::string& occurrenceName) noexcept override;

   void populateClassLifecycleStats(std::unordered_map<std::string, LifecycleStats>& mapClassLifecycleStats);
   void populateOccurrences(std::unordered_map<std::string, std::unordered_map<std::string, long long>>& mapOccurrences);
   
   const std::string& logLevelPrefix(LogLevel level) const noexcept;

   // disallow copies
   StdLogger(const StdLogger&) = delete;
   StdLogger(StdLogger&&) = delete;
   StdLogger& operator=(const StdLogger&) = delete;
   StdLogger& operator=(StdLogger&&) = delete;

private:
   std::unordered_map<std::string, LifecycleStats> m_mapClassLifecycleStats;
   std::unordered_map<std::string, std::unordered_map<std::string, long long>> m_mapOccurrences;
   std::unique_ptr<Mutex> m_lockLifecycleStats;
   std::unique_ptr<Mutex> m_lockOccurrences;
   LogLevel m_logLevel;
   bool m_isLoggingInstanceLifecycles;
   
   static const std::string prefixCritical;
   static const std::string prefixError;
   static const std::string prefixWarning;
   static const std::string prefixInfo;
   static const std::string prefixDebug;
   static const std::string prefixVerbose;
};


#endif /* defined(__HttpServer__StdLogger__) */
