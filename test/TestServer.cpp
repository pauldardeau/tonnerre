#include <string>

#include "MessagingServer.h"
#include "MessageHandlerAdapter.h"
#include "Message.h"
#include "KeyValuePairs.h"
#include "SystemInfo.h"

//******************************************************************************
//******************************************************************************
                                    
class TestServerInfo : public MessageHandlerAdapter
{
public:
   void handleTextMessage(std::shared_ptr<Message> requestMessage,
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
   void handleKeyValuesMessage(std::shared_ptr<Message> requestMessage,
                               Message& responseMessage,
                               const std::string& requestName,
                               const KeyValuePairs& requestPayload,
                               KeyValuePairs& responsePayload)
   {
      if (requestName == "echo") {
         
      }                               
   }
};

//******************************************************************************
//******************************************************************************

class StoogeInfoService : public MessageHandlerAdapter
{
public:
   void handleKeyValuesMessage(std::shared_ptr<Message> requestMessage,
                               Message& responseMessage,
                               const std::string& requestName,
                               const KeyValuePairs& requestPayload,
                               KeyValuePairs& responsePayload)
   {
      if (requestName == "listStooges") {
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
   std::string serviceName;
   serviceName = "ServerInfo";
   serviceName = "EchoService";
   serviceName = "StoogeInfoService";
   MessageHandler* handler = nullptr;
   
   if (serviceName == "ServerInfo") {
      handler = new TestServerInfo();
   } else if (serviceName == "EchoService") {
      handler = new TestEchoService();
   } else if (serviceName == "StoogeInfoService") {
      handler = new StoogeInfoService();
   }
   
   if (handler != nullptr) {
      MessagingServer("tonnerre.ini", serviceName, handler);
   }
}

//******************************************************************************
