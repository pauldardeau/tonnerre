// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <memory>


/**
 * Logger is an interface (abstract base class) for logging.
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
   
   /**
    * Destructor
    */
   virtual ~Logger() noexcept {}
   
   /**
    * Retrieves the current log level
    * @return the current log level value
    */
   virtual LogLevel getLogLevel() const noexcept = 0;
   
   /**
    * Sets the current log level
    * @param logLevel the new log level
    */
   virtual void setLogLevel(LogLevel logLevel) noexcept = 0;
   
   /**
    *
    * @return
    */
   virtual bool isLoggingInstanceLifecycles() const noexcept = 0;
   
   /**
    *
    * @param logInstanceLifecycles
    */
   virtual void setLogInstanceLifecycles(bool logInstanceLifecycles) noexcept = 0;
   
   /**
    *
    * @param className
    */
   virtual void logInstanceCreate(const std::string& className) noexcept = 0;
   
   /**
    *
    * @param className
    */
   virtual void logInstanceDestroy(const std::string& className) noexcept = 0;
   
   /**
    *
    * @param logLevel
    * @param logMessage
    */
   virtual void logMessage(LogLevel logLevel, const std::string& logMessage) noexcept = 0;
   
   /**
    *
    * @param logLevel
    * @return
    */
   virtual bool isLoggingLevel(LogLevel logLevel) const noexcept = 0;
   
   /**
    *
    * @param occurrenceType
    * @param occurrenceName
    */
   virtual void logOccurrence(const std::string& occurrenceType,
                              const std::string& occurrenceName) noexcept = 0;
   
   /**
    *
    * @param logger
    */
   static void setLogger(std::shared_ptr<Logger> logger) noexcept;
   
   /**
    *
    * @return
    */
   static std::shared_ptr<Logger> getLogger() noexcept;
   
   /**
    *
    * @param logLevel
    * @param logMessage
    */
   static void log(LogLevel logLevel, const std::string& logMessage) noexcept;
   
   /**
    *
    * @param logMessage
    */
   static void critical(const std::string& logMessage) noexcept;
   
  /**
   *
   * @param logMessage
   */
   static void error(const std::string& logMessage) noexcept;
   
  /**
   *
   * @param logMessage
   */
   static void warning(const std::string& logMessage) noexcept;
   
  /**
   *
   * @param logMessage
   */
   static void info(const std::string& logMessage) noexcept;
   
  /**
   *
   * @param logMessage
   */
   static void debug(const std::string& logMessage) noexcept;
   
  /**
   *
   * @param logMessage
   */
   static void verbose(const std::string& logMessage) noexcept;
   
   /**
    *
    * @param logLevel
    * @return
    */
   static bool isLogging(LogLevel logLevel) noexcept;
   
   // object lifecycles (create/destroy objects)
   /**
    *
    * @param className
    */
   static void logInstanceCreate(const char* className) noexcept;
   
   /**
    *
    * @param className
    */
   static void logInstanceDestroy(const char* className) noexcept;

   // counting occurrences
   /**
    *
    * @param occurrenceType
    * @param occurrence
    */
   static void countOccurrence(const char* occurrenceType,
                               const char* occurrence) noexcept;
                               
   /**
    *
    * @param occurrenceType
    * @param occurrence
    */
   static void countOccurrence(const std::string& occurrenceType,
                               const std::string& occurrence) noexcept;
   
   
private:
   static std::shared_ptr<Logger> loggerInstance;
   
};


#endif
