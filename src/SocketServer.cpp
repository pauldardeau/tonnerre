// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#include <string>
#include <exception>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/time.h>

#include "SocketServer.h"

// sockets
#include "ServerSocket.h"
#include "Socket.h"

#include "RequestHandler.h"

// utils
#include "BasicException.h"
#include "IniReader.h"
#include "KeyValuePairs.h"
#include "StrUtils.h"
#include "Logger.h"
#include "SystemInfo.h"

// threading
#include "Mutex.h"
#include "Runnable.h"
#include "Thread.h"
#include "ThreadPoolDispatcher.h"
#include "ThreadingFactory.h"

// kernel events
#include "EpollServer.h"
#include "KqueueServer.h"



static const std::string SERVER_NAME             = "Misere";
static const std::string SERVER_VERSION          = "0.1";

static const std::string CFG_TRUE_SETTING_VALUES = "yes|true|1";

static const std::string EMPTY = "";
static const std::string SPACE = " ";
static const std::string EOL   = "\n";
static const std::string COLON = ":";


// default settings
static const int CFG_DEFAULT_SEND_BUFFER_SIZE     = 8192;
static const int CFG_DEFAULT_RECEIVE_BUFFER_SIZE  = 8192;

static const int CFG_DEFAULT_PORT_NUMBER          = 9000;

static const int CFG_DEFAULT_THREAD_POOL_SIZE     = 4;


// configuration sections
static const std::string CFG_SECTION_SERVER                 = "server";
static const std::string CFG_SECTION_LOGGING                = "logging";

// logging config values
static const std::string CFG_LOGFILE_ACCESS                 = "access_log";
static const std::string CFG_LOGFILE_ERROR                  = "error_log";

// server config values
static const std::string CFG_SERVER_PORT                    = "port";
static const std::string CFG_SERVER_THREADING               = "threading";
static const std::string CFG_SERVER_THREAD_POOL_SIZE        = "thread_pool_size";
static const std::string CFG_SERVER_LOG_LEVEL               = "log_level";
static const std::string CFG_SERVER_SEND_BUFFER_SIZE        = "socket_send_buffer_size";
static const std::string CFG_SERVER_RECEIVE_BUFFER_SIZE     = "socket_receive_buffer_size";
static const std::string CFG_SERVER_ALLOW_BUILTIN_HANDLERS  = "allow_builtin_handlers";
static const std::string CFG_SERVER_STRING                  = "server_string";
static const std::string CFG_SERVER_SOCKETS                 = "sockets";

// socket options
static const std::string CFG_SOCKETS_SOCKET_SERVER          = "socket_server";
static const std::string CFG_SOCKETS_KERNEL_EVENTS          = "kernel_events";


// threading options
static const std::string CFG_THREADING_PTHREADS             = "pthreads";
static const std::string CFG_THREADING_CPP11                = "c++11";
static const std::string CFG_THREADING_GCD_LIBDISPATCH      = "gcd_libdispatch";
static const std::string CFG_THREADING_NONE                 = "none";

// logging level options
static const std::string CFG_LOGGING_CRITICAL               = "critical";
static const std::string CFG_LOGGING_ERROR                  = "error";
static const std::string CFG_LOGGING_WARNING                = "warning";
static const std::string CFG_LOGGING_INFO                   = "info";
static const std::string CFG_LOGGING_DEBUG                  = "debug";
static const std::string CFG_LOGGING_VERBOSE                = "verbose";

// mime types
static const std::string MIME_APPLICATION_JSON  = "application/json";
static const std::string MIME_APPLICATION_XML   = "application/xml";
static const std::string MIME_TEXT_HTML         = "text/html";
static const std::string MIME_TEXT_PLAIN        = "text/plain";


// module config values
static const std::string APP_PREFIX = "app:";

static const auto APP_PREFIX_LEN = APP_PREFIX.length();

static const char* LOG_WEEKDAY_NAME[7] =
{
   "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

static const char* LOG_MONTH_NAME[12] =
{
   "Jan", "Feb", "Mar", "Apr", "May", "Jun",
   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};


//******************************************************************************
//******************************************************************************

SocketServer::SocketServer(const std::string& configFilePath) :
   m_kernelEventServer(nullptr),
   m_serverSocket(nullptr),
   m_threadPool(nullptr),
   m_threadingFactory(nullptr),
   m_configFilePath(configFilePath),
   m_isDone(false),
   m_isThreaded(true),
   m_isUsingKernelEventServer(false),
   m_isFullyInitialized(false),
   m_compressionEnabled(true),
   m_threadPoolSize(CFG_DEFAULT_THREAD_POOL_SIZE),
   m_serverPort(CFG_DEFAULT_PORT_NUMBER),
   m_minimumCompressionSize(1000)
{
   Logger::logInstanceCreate("SocketServer");

   init(CFG_DEFAULT_PORT_NUMBER);
}

//******************************************************************************

std::unique_ptr<SectionedConfigDataSource> SocketServer::getConfigDataSource()
{
   return std::unique_ptr<SectionedConfigDataSource>(new IniReader(m_configFilePath));
}

//******************************************************************************

int SocketServer::getSocketSendBufferSize() const noexcept
{
   return m_socketSendBufferSize;
}

//******************************************************************************

int SocketServer::getSocketReceiveBufferSize() const noexcept
{
   return m_socketReceiveBufferSize;
}

//******************************************************************************

const std::string& SocketServer::getServerId() const noexcept
{
   return m_serverString;
}

//******************************************************************************

bool SocketServer::hasTrueValue(const KeyValuePairs& kvp,
                              const std::string& setting) const noexcept
{
   bool hasTrueValue = false;
   
   if (kvp.hasKey(setting)) {
      const std::string& settingValue = kvp.getValue(setting);
      
      if (StrUtils::containsString(CFG_TRUE_SETTING_VALUES, settingValue)) {
         hasTrueValue = true;
      }
   }
   
   return hasTrueValue;
}

//******************************************************************************

int SocketServer::getIntValue(const KeyValuePairs& kvp,
                            const std::string& setting) const noexcept
{
   int value = -1;
   
   if (kvp.hasKey(setting)) {
      const std::string& valueAsString = kvp.getValue(setting);
      const int intValue = std::stoi(valueAsString);
      
      if (intValue > 0) {
         value = intValue;
      }
   }
   
   return value;
}

//******************************************************************************

void SocketServer::replaceVariables(const KeyValuePairs& kvp,
                                  std::string& s) const noexcept
{
   if (!s.empty()) {
      std::vector<std::string> keys;
      kvp.getKeys(keys);
      
      for (const std::string& key : keys) {
         if (StrUtils::containsString(s, key)) {
            StrUtils::replaceAll(s, key, kvp.getValue(key));
         }
      }
   }
}

//******************************************************************************

bool SocketServer::init(int port)
{
   const bool isLoggingDebug = Logger::isLogging(Logger::LogLevel::Debug);
   
	m_serverPort = port;
	
   std::unique_ptr<SectionedConfigDataSource> configDataSource;
   
   try {
      configDataSource = std::move(getConfigDataSource());
   }
   catch (const BasicException& be)
   {
      Logger::error("exception retrieving config data: " + be.whatString());
      return false;
   }
   catch (const std::exception& e)
   {
      Logger::error("exception retrieving config data: " + std::string(e.what()));
      return false;
   }
   catch (...)
   {
      Logger::error("exception retrieving config data");
      return false;
   }
   
   if (!configDataSource) {
      Logger::error("unable to retrieve config data");
      return false;
   }

   // start out with our default settings
   m_socketSendBufferSize = CFG_DEFAULT_SEND_BUFFER_SIZE;
   m_socketReceiveBufferSize = CFG_DEFAULT_RECEIVE_BUFFER_SIZE;

   try {
      KeyValuePairs kvpServerSettings;

      // read and process "server" section
      if (configDataSource->hasSection(CFG_SECTION_SERVER) &&
          configDataSource->readSection(CFG_SECTION_SERVER, kvpServerSettings)) {
         
         if (kvpServerSettings.hasKey(CFG_SERVER_PORT)) {
            const int portNumber =
               getIntValue(kvpServerSettings, CFG_SERVER_PORT);

            if (portNumber > 0) {
               port = portNumber;
					m_serverPort = portNumber;
               
               if (isLoggingDebug) {
                  char msg[128];
                  std::snprintf(msg, 128, "port number=%d", port);
                  Logger::debug(std::string(msg));
               }
            }
         }

         // defaults
         m_isThreaded = true;
         m_threading = CFG_THREADING_PTHREADS;
         m_threadPoolSize = 4;
         
         if (kvpServerSettings.hasKey(CFG_SERVER_THREADING)) {
            const std::string& threading =
               kvpServerSettings.getValue(CFG_SERVER_THREADING);
            if (!threading.empty()) {
               if ((threading == CFG_THREADING_PTHREADS) ||
                   (threading == CFG_THREADING_CPP11) ||
                   (threading == CFG_THREADING_GCD_LIBDISPATCH)) {
                  m_threading = threading;
                  m_isThreaded = true;
               } else if (threading == CFG_THREADING_NONE) {
                  m_isThreaded = false;
               }
            }
         }
         
         if (kvpServerSettings.hasKey(CFG_SERVER_THREAD_POOL_SIZE)) {
            const int poolSize =
               getIntValue(kvpServerSettings, CFG_SERVER_THREAD_POOL_SIZE);

            if (poolSize > 0) {
               m_threadPoolSize = poolSize;
            }
         }
         
         // defaults
         m_sockets = CFG_SOCKETS_SOCKET_SERVER;
         
         if (kvpServerSettings.hasKey(CFG_SERVER_SOCKETS)) {
            const std::string& sockets = kvpServerSettings.getValue(CFG_SERVER_SOCKETS);
            if (sockets == CFG_SOCKETS_KERNEL_EVENTS) {
               m_isUsingKernelEventServer = true;
               m_sockets = CFG_SOCKETS_KERNEL_EVENTS;
            }
         }

         if (kvpServerSettings.hasKey(CFG_SERVER_LOG_LEVEL)) {
            m_logLevel =
               kvpServerSettings.getValue(CFG_SERVER_LOG_LEVEL);
            if (!m_logLevel.empty()) {
               StrUtils::toLowerCase(m_logLevel);
               Logger::info(std::string("log level: ") + m_logLevel);
               std::shared_ptr<Logger> logger = Logger::getLogger();
               
               if (logger != nullptr) {
                  if (m_logLevel == CFG_LOGGING_CRITICAL) {
                     logger->setLogLevel(Logger::LogLevel::Critical);
                  } else if (m_logLevel == CFG_LOGGING_ERROR) {
                     logger->setLogLevel(Logger::LogLevel::Error);
                  } else if (m_logLevel == CFG_LOGGING_WARNING) {
                     logger->setLogLevel(Logger::LogLevel::Warning);
                  } else if (m_logLevel == CFG_LOGGING_INFO) {
                     logger->setLogLevel(Logger::LogLevel::Info);
                  } else if (m_logLevel == CFG_LOGGING_DEBUG) {
                     logger->setLogLevel(Logger::LogLevel::Debug);
                  } else if (m_logLevel == CFG_LOGGING_VERBOSE) {
                     logger->setLogLevel(Logger::LogLevel::Verbose);
                  } else {
                     Logger::warning("unrecognized log level: '" + m_logLevel);
                  }
               }
            }
         }

         if (kvpServerSettings.hasKey(CFG_SERVER_SEND_BUFFER_SIZE)) {
            const int buffSize =
               getIntValue(kvpServerSettings, CFG_SERVER_SEND_BUFFER_SIZE);

            if (buffSize > 0) {
               m_socketSendBufferSize = buffSize;
            }
         }

         if (kvpServerSettings.hasKey(CFG_SERVER_RECEIVE_BUFFER_SIZE)) {
            const int buffSize =
               getIntValue(kvpServerSettings, CFG_SERVER_RECEIVE_BUFFER_SIZE);

            if (buffSize > 0) {
               m_socketReceiveBufferSize = buffSize;
            }
         }
         
         if (kvpServerSettings.hasKey(CFG_SERVER_STRING)) {
            const std::string& serverString =
               kvpServerSettings.getValue(CFG_SERVER_STRING);
            if (!serverString.empty()) {
               m_serverString = serverString;

               const auto posDollar = serverString.find("$");
               if (posDollar != std::string::npos) {
                  KeyValuePairs kvpVars;
                  kvpVars.addPair("$PRODUCT_NAME", SERVER_NAME);
                  kvpVars.addPair("$PRODUCT_VERSION", SERVER_VERSION);
                  kvpVars.addPair("$CFG_SOCKETS", m_sockets);
                  kvpVars.addPair("$CFG_THREADING", m_threading);
                  
                  const auto posDollarOS = serverString.find("$OS_");
                  
                  if (posDollarOS != std::string::npos) {
                     SystemInfo systemInfo;
                     if (systemInfo.retrievedSystemInfo()) {
                        kvpVars.addPair("$OS_SYSNAME", systemInfo.sysName());
                        kvpVars.addPair("$OS_NODENAME", systemInfo.nodeName());
                        kvpVars.addPair("$OS_RELEASE", systemInfo.release());
                        kvpVars.addPair("$OS_VERSION", systemInfo.version());
                        kvpVars.addPair("$OS_MACHINE", systemInfo.machine());
                     } else {
                        Logger::warning("unable to retrieve system information to populate server string");
                     }
                  }
                  
                  replaceVariables(kvpVars, m_serverString);
               }
               
               Logger::info("setting server string: '" + m_serverString + "'");
            }
         }
      }

		m_startupTime = getLocalDateTime();
   }
   catch (const BasicException& be)
   {
      Logger::critical("exception initializing server: " + be.whatString());
      return false;
   }
   catch (const std::exception& e)
   {
      Logger::critical("exception initializing server: " +
                       std::string(e.what()));
      return false;
   }
   catch (...)
   {
      Logger::critical("unknown exception initializing server");
      return false;
   }


   if (!m_isUsingKernelEventServer) {
      try {
         if (isLoggingDebug) {
            char msg[128];
            std::snprintf(msg, 128, "creating server socket on port=%d", port);
            Logger::debug(std::string(msg));
         }
      
         std::unique_ptr<ServerSocket> serverSocket(new ServerSocket(port));
         m_serverSocket = std::move(serverSocket);
      }
      catch (...)
      {
         std::string exception = "unable to open server socket port '";
         exception += std::to_string(port);
         exception += "'";
         Logger::critical(exception);
         return false;
      }
   }


   std::string concurrencyModel = EMPTY;

   if (m_isThreaded) {
      ThreadingFactory::ThreadingPackage threadingPackage;
      bool isUsingLibDispatch = false;
      
      if (m_threading == CFG_THREADING_PTHREADS) {
         threadingPackage = ThreadingFactory::ThreadingPackage::PTHREADS;
      } else if (m_threading == CFG_THREADING_CPP11) {
         threadingPackage = ThreadingFactory::ThreadingPackage::CPP_11;
      } else if (m_threading == CFG_THREADING_GCD_LIBDISPATCH) {
         threadingPackage = ThreadingFactory::ThreadingPackage::GCD_LIBDISPATCH;
         isUsingLibDispatch = true;
      } else {
         threadingPackage = ThreadingFactory::ThreadingPackage::PTHREADS;
      }
      
      std::shared_ptr<ThreadingFactory>
         threadingFactory(new ThreadingFactory(threadingPackage));
      ThreadingFactory::setThreadingFactory(threadingFactory);
      m_threadingFactory = std::move(threadingFactory);
      
      if (m_isUsingKernelEventServer) {
         m_threadingFactory->setMutexType(ThreadingFactory::ThreadingPackage::PTHREADS);
      }
         
      m_threadPool = m_threadingFactory->createThreadPoolDispatcher(m_threadPoolSize);
         
      m_threadPool->start();

      concurrencyModel = "multithreaded - ";
      concurrencyModel += m_threading;
      
      if (!isUsingLibDispatch) {
         char numberThreads[128];
         std::snprintf(numberThreads, 128, " [%d threads]",
                       m_threadPoolSize);
         concurrencyModel += numberThreads;
      }
   } else {
      concurrencyModel = "serial";
		m_threadPoolSize = 1;   // not a pool, we have 1 processing thread
   }

	m_concurrencyModel = concurrencyModel;

   std::string portAsString = std::to_string(port);

   std::string startupMsg = SERVER_NAME;
   startupMsg += " ";
   startupMsg += SERVER_VERSION;
   startupMsg += " listening on port ";
   startupMsg += portAsString;
   startupMsg += " (request concurrency: ";
   startupMsg += concurrencyModel;
   startupMsg += ")";
   startupMsg += " (sockets: ";
   startupMsg += m_sockets;
   startupMsg += ")";

   std::printf("%s\n", startupMsg.c_str());

   m_isFullyInitialized = true;
   
   return true;
}

//******************************************************************************

SocketServer::~SocketServer() noexcept
{
   Logger::logInstanceDestroy("SocketServer");

   if (m_serverSocket) {
      m_serverSocket->close();
   }

   if (m_threadPool) {
      m_threadPool->stop();
   }
}

//******************************************************************************

std::string SocketServer::getSystemDateGMT() const noexcept
{
   time_t currentGMT;
   ::time(&currentGMT);
   
   struct tm* timeptr = ::gmtime(&currentGMT);
   char dateBuffer[128];
   
   std::snprintf(dateBuffer, 128, "%.3s, %02d %.3s %d %.2d:%.2d:%.2d GMT",
             LOG_WEEKDAY_NAME[timeptr->tm_wday],
             timeptr->tm_mday,
             LOG_MONTH_NAME[timeptr->tm_mon],
             1900 + timeptr->tm_year,
             timeptr->tm_hour,
             timeptr->tm_min,
             timeptr->tm_sec);
   
   return std::string(dateBuffer);
}

//******************************************************************************

std::string SocketServer::getLocalDateTime() const noexcept
{
   time_t currentTime;
   ::time(&currentTime);
   
   struct tm* timeptr = ::localtime(&currentTime);
   char dateBuffer[128];
   
   std::snprintf(dateBuffer, 128, "%d-%02d-%02d %.2d:%.2d:%.2d",
             1900 + timeptr->tm_year,
             timeptr->tm_mon + 1,
             timeptr->tm_mday,
             timeptr->tm_hour,
             timeptr->tm_min,
             timeptr->tm_sec);
   
   return std::string(dateBuffer);
}

//******************************************************************************

bool SocketServer::compressResponse(const std::string& mimeType) const noexcept
{
   //TODO: make this configurable through config file
   return (mimeType == MIME_TEXT_HTML) ||
          (mimeType == MIME_TEXT_PLAIN) ||
          (mimeType == MIME_APPLICATION_JSON) ||
          (mimeType == MIME_APPLICATION_XML);
}

//******************************************************************************

bool SocketServer::compressionEnabled() const noexcept
{
   return m_compressionEnabled;
}

//******************************************************************************

int SocketServer::minimumCompressionSize() const noexcept
{
   return m_minimumCompressionSize;
}

//******************************************************************************

int SocketServer::platformPointerSizeBits() const noexcept
{
   return sizeof(void*) * 8;
}

//******************************************************************************

void SocketServer::serviceSocket(std::shared_ptr<SocketRequest> socketRequest)
{
   if (nullptr != m_threadPool) {
      // Hand off the request to the thread pool for asynchronous processing
      std::shared_ptr<RequestHandler> requestHandler = handlerForSocketRequest(socketRequest);
      requestHandler->setThreadPooling(true);
      m_threadPool->addRequest(requestHandler);
   } else {
      // no thread pool available -- process it synchronously
      std::shared_ptr<RequestHandler> requestHandler = handlerForSocketRequest(socketRequest);
      requestHandler->run();
   }
}

//******************************************************************************

int SocketServer::runSocketServer() noexcept
{
   int rc = 0;
   
   if (!m_serverSocket) {
      Logger::critical("runSocketServer called with null serverSocket");
      return 1;
   }
   
   while (!m_isDone) {
      
      std::shared_ptr<Socket> socket(m_serverSocket->accept());

      if (nullptr == socket) {
         continue;
      }

      if (Logger::isLogging(Logger::LogLevel::Debug)) {
         Logger::debug("*****************************************");
         Logger::debug("client connected");
      }

      try {
         
         if (m_isThreaded && (nullptr != m_threadPool)) {
            std::shared_ptr<RequestHandler> handler = handlerForSocket(socket);

            handler->setThreadPooling(true);

            // give it to the thread pool
            m_threadPool->addRequest(handler);
         } else {
            std::shared_ptr<RequestHandler> handler = handlerForSocket(socket);
            handler->run();
         }
      }
      catch (const BasicException& be)
      {
         rc = 1;
         Logger::error("SocketServer runServer exception caught: " +
                       be.whatString());
      }
      catch (const std::exception& e)
      {
         rc = 1;
         Logger::error(std::string("SocketServer runServer exception caught: ") +
                       std::string(e.what()));
      }
      catch (...)
      {
         rc = 1;
         Logger::error("SocketServer runServer unknown exception caught");
      }
   }
   
   return rc;
}

//******************************************************************************

int SocketServer::runKernelEventServer() noexcept
{
   const int MAX_CON = 1200;
   
   int rc = 0;
   
   if (m_threadingFactory != nullptr) {
      std::shared_ptr<Mutex> mutexFD(m_threadingFactory->createMutex("fdMutex"));
      std::shared_ptr<Mutex> mutexHWMConnections(m_threadingFactory->createMutex("hwmConnectionsMutex"));
      std::unique_ptr<KernelEventServer> kernelEventServer = nullptr;
      
      if (KqueueServer::isSupportedPlatform()) {
         kernelEventServer =
            std::unique_ptr<KernelEventServer>(new KqueueServer(*mutexFD, *mutexHWMConnections));
      } else if (EpollServer::isSupportedPlatform()) {
         kernelEventServer =
            std::unique_ptr<KernelEventServer>(new EpollServer(*mutexFD, *mutexHWMConnections));
      } else {
         Logger::critical("no kernel event server available for platform");
         rc = 1;
      }
      
      if (kernelEventServer != nullptr) {
         try {
            std::shared_ptr<SocketServiceHandler> serviceHandler(createSocketServiceHandler());

            if (kernelEventServer->init(serviceHandler, m_serverPort, MAX_CON))
            {
               kernelEventServer->run();
            } else {
               rc = 1;
            }
         } catch (const BasicException& be) {
            Logger::critical("exception running kernel event server: " +
                             be.whatString());
         } catch (const std::exception& e) {
            Logger::critical("exception running kernel event server: " +
                             std::string(e.what()));
         } catch (...) {
            Logger::critical("unidentified exception running kernel event server");
         }
      }
   } else {
      Logger::critical("no threading factory configured");
      rc = 1;
   }
   
   return rc;
}

//******************************************************************************

int SocketServer::run() noexcept
{
   if (!m_isFullyInitialized) {
      Logger::critical("server not initialized");
      return 1;
   } else {
      if (m_isUsingKernelEventServer) {
         return runKernelEventServer();
      } else {
         return runSocketServer();
      }
   }
}

//******************************************************************************


