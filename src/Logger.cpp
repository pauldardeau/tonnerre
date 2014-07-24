// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#include "Logger.h"

std::shared_ptr<Logger> Logger::loggerInstance = nullptr;

//******************************************************************************

void Logger::setLogger(std::shared_ptr<Logger> logger) noexcept
{
   loggerInstance = logger;
}

//******************************************************************************

std::shared_ptr<Logger> Logger::getLogger() noexcept
{
   return loggerInstance;
}

//******************************************************************************

void Logger::log(LogLevel logLevel, const std::string& logMessage) noexcept
{
   if (loggerInstance) {
      loggerInstance->logMessage(logLevel, logMessage);
   }
}

//******************************************************************************

void Logger::critical(const std::string& logMessage) noexcept
{
   log(LogLevel::Critical, logMessage);
}

//******************************************************************************

void Logger::error(const std::string& logMessage) noexcept
{
   log(LogLevel::Error, logMessage);
}

//******************************************************************************

void Logger::warning(const std::string& logMessage) noexcept
{
   log(LogLevel::Warning, logMessage);
}

//******************************************************************************

void Logger::info(const std::string& logMessage) noexcept
{
   log(LogLevel::Info, logMessage);
}

//******************************************************************************

void Logger::debug(const std::string& logMessage) noexcept
{
   log(LogLevel::Debug, logMessage);
}

//******************************************************************************

void Logger::verbose(const std::string& logMessage) noexcept
{
   log(LogLevel::Verbose, logMessage);
}

//******************************************************************************

bool Logger::isLogging(LogLevel logLevel) noexcept
{
   if (loggerInstance) {
      return loggerInstance->isLoggingLevel(logLevel);
   }
   
   return false;
}

//******************************************************************************

void Logger::logInstanceCreate(const char* className) noexcept
{
   if (loggerInstance) {
      if (loggerInstance->isLoggingInstanceLifecycles()) {
         loggerInstance->logInstanceCreate(std::string(className));
      }
   }
}

//******************************************************************************

void Logger::logInstanceDestroy(const char* className) noexcept
{
   if (loggerInstance) {
      if (loggerInstance->isLoggingInstanceLifecycles()) {
         loggerInstance->logInstanceDestroy(std::string(className));
      }
   }
}

//******************************************************************************

void Logger::countOccurrence(const char* occurrenceType,
                             const char* occurrenceName) noexcept
{
   if (loggerInstance) {
      loggerInstance->logOccurrence(std::string(occurrenceType),
                                    std::string(occurrenceName));
   }
}

//******************************************************************************

void Logger::countOccurrence(const std::string& occurrenceType,
                             const std::string& occurrenceName) noexcept
{
   if (loggerInstance) {
      loggerInstance->logOccurrence(occurrenceType, occurrenceName);
   }
}

//******************************************************************************

