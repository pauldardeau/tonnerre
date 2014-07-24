// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef HttpServer_Logger_h
#define HttpServer_Logger_h

#include <string>
#include <memory>


/*!
 * Logger is an abstract base class for logging.
 */
class Logger
{
public:
   enum LogLevel
   {
      Critical = 0,  // least verbose
      Error    = 1,
      Warning  = 2,
      Info     = 3,
      Debug    = 4,
      Verbose  = 5   // most verbose
   };
   
   virtual ~Logger() noexcept {}
   
   virtual LogLevel getLogLevel() const noexcept = 0;
   virtual void setLogLevel(LogLevel logLevel) noexcept = 0;
   
   virtual bool isLoggingInstanceLifecycles() const noexcept = 0;
   virtual void setLogInstanceLifecycles(bool logInstanceLifecycles) noexcept = 0;
   virtual void logInstanceCreate(const std::string& className) noexcept = 0;
   virtual void logInstanceDestroy(const std::string& className) noexcept = 0;
   
   virtual void logMessage(LogLevel logLevel, const std::string& logMessage) noexcept = 0;
   virtual bool isLoggingLevel(LogLevel logLevel) const noexcept = 0;
   
   virtual void logOccurrence(const std::string& occurrenceType,
                              const std::string& occurrenceName) noexcept = 0;
   
   static void setLogger(std::shared_ptr<Logger> logger) noexcept;
   static std::shared_ptr<Logger> getLogger() noexcept;
   static void log(LogLevel logLevel, const std::string& logMessage) noexcept;
   static void critical(const std::string& logMessage) noexcept;
   static void error(const std::string& logMessage) noexcept;
   static void warning(const std::string& logMessage) noexcept;
   static void info(const std::string& logMessage) noexcept;
   static void debug(const std::string& logMessage) noexcept;
   static void verbose(const std::string& logMessage) noexcept;
   static bool isLogging(LogLevel logLevel) noexcept;
   
   // object lifecycles (create/destroy objects)
   static void logInstanceCreate(const char* className) noexcept;
   static void logInstanceDestroy(const char* className) noexcept;

   // counting occurrences
   static void countOccurrence(const char* occurrenceType,
                               const char* occurrence) noexcept;
   static void countOccurrence(const std::string& occurrenceType,
                               const std::string& occurrence) noexcept;
   
   
private:
   static std::shared_ptr<Logger> loggerInstance;
   
};


#endif
