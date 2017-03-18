// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#include <stdio.h>
#include <string>

#include "MessagingServer.h"
#include "MessageHandlerAdapter.h"
#include "Message.h"
#include "KeyValuePairs.h"
#include "SystemInfo.h"
#include "Logger.h"
#include "StdLogger.h"

using namespace std;
using namespace tonnerre;
using namespace chaudiere;

//******************************************************************************
//******************************************************************************
                                    
class TestServerInfo : public MessageHandlerAdapter
{
public:
   void handleTextMessage(const Message& requestMessage,
                          Message& responseMessage,
                          const std::string& requestName,
                          const std::string& requestPayload,
                          std::string& responsePayload)
   {
      if (requestName == "serverInfo") {
         SystemInfo systemInfo;
         if (systemInfo.retrievedSystemInfo()) {
            responsePayload += "sysName=";
            responsePayload += systemInfo.sysName();
            responsePayload += ";";
            responsePayload += "nodeName=";
            responsePayload += systemInfo.nodeName();
            responsePayload += ";";
            responsePayload += "release=";
            responsePayload += systemInfo.release();
            responsePayload += ";";
            responsePayload += "version=";
            responsePayload += systemInfo.version();
            responsePayload += ";";
            responsePayload += "machine=";
            responsePayload += systemInfo.machine();
         } else {
            responsePayload += "error: unable to retrieve server information";
         }
      }
   }
};

//******************************************************************************
//******************************************************************************

class TestEchoService : public MessageHandlerAdapter
{
public:
   void handleKeyValuesMessage(const Message& requestMessage,
                               Message& responseMessage,
                               const std::string& requestName,
                               const KeyValuePairs& requestPayload,
                               KeyValuePairs& responsePayload)
   {
      if (requestName == "echo") {
         responsePayload = requestPayload;
      }                               
   }
};

//******************************************************************************
//******************************************************************************

class StoogeInfoService : public MessageHandlerAdapter
{
public:
   void handleKeyValuesMessage(const Message& requestMessage,
                               Message& responseMessage,
                               const std::string& requestName,
                               const KeyValuePairs& requestPayload,
                               KeyValuePairs& responsePayload)
   {
      printf("StoogesInfoServer.handleKeyValuesMessage called\n");
      
      if (requestName == "listStooges") {
         printf("listStooges request\n");
         
         responsePayload.addPair("stooge1", "Moe");
         responsePayload.addPair("stooge2", "Larry");
         responsePayload.addPair("stooge3", "Curly");
      }                               
   }
};

//******************************************************************************
//******************************************************************************

int main(int argc, char* argv[])
{
   const string SERVICE_SERVER_INFO = "server_info";
   const string SERVICE_ECHO        = "echo_service";
   const string SERVICE_STOOGE_INFO = "stooge_info_service";

   string serviceName;
   //serviceName = SERVICE_SERVER_INFO;
   serviceName = SERVICE_ECHO;
   //serviceName = SERVICE_STOOGE_INFO;
   MessageHandler* handler = NULL;
   
   StdLogger* logger = new StdLogger(Info);
   //logger->setLogInstanceLifecycles(true);
   Logger::setLogger(logger);
   
   if (serviceName == SERVICE_SERVER_INFO) {
      handler = new TestServerInfo();
   } else if (serviceName == SERVICE_ECHO) {
      handler = new TestEchoService();
   } else if (serviceName == SERVICE_STOOGE_INFO) {
      handler = new StoogeInfoService();
   }
   
   if (handler != NULL) {
      MessagingServer server("tonnerre.ini", serviceName, handler);
      server.run();
      delete handler;
   }
}

//******************************************************************************
