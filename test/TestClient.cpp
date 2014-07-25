#include <string>
#include <vector>
#include <cstdio>

#include "Messaging.h"
#include "Message.h"
#include "KeyValuePairs.h"
#include "BasicException.h"

//******************************************************************************

void PrintKeyValues(const KeyValuePairs& kvp)
{
   std::vector<std::string> keys;
   kvp.getKeys(keys);
   for (auto key : keys) {
      auto value = kvp.getValue(key);
      std::printf("key='%s', value='%s'\n", key.c_str(), value.c_str());
   }
}

//******************************************************************************

int main(int argc, char* argv[])
{
   std::string serviceName;
   serviceName = "ServerInfo";
   serviceName = "EchoService";
   serviceName = "StoogeInfoService";

   try {
      Messaging::initialize("tonnerre.ini");
   
      if (serviceName == "ServerInfo") {
         Message message("serverInfo", Message::MessageType::Text);
         Message response;
         if (message.send(serviceName, response)) {
            const std::string& responseText = response.getTextPayload();
            std::printf("response: '%s'\n", responseText.c_str());
         } else {
            std::printf("error: unable to send message to service '%s'\n", serviceName.c_str());
         }
      } else if (serviceName == "EchoService") {
         KeyValuePairs kvp;
         kvp.addPair("firstName", "Mickey");
         kvp.addPair("lastName", "Mouse");
         kvp.addPair("city", "Orlando");
         kvp.addPair("state", "FL");

         Message message("echo", Message::MessageType::KeyValues);
         message.setKeyValuesPayload(kvp);
         Message response;
         if (message.send(serviceName, response)) {
            const KeyValuePairs& responseKeyValues = response.getKeyValuesPayload();
            PrintKeyValues(responseKeyValues);
         } else {
            std::printf("error: unable to send message to service '%s'\n", serviceName.c_str());
         }
      } else if (serviceName == "StoogeInfoService") {
         Message message("listStooges", Message::MessageType::KeyValues);
         Message response;
         if (message.send(serviceName, response)) {
            const KeyValuePairs& responseKeyValues = response.getKeyValuesPayload();
            PrintKeyValues(responseKeyValues);
         } else {
            std::printf("error: unable to send message to service '%s'\n", serviceName.c_str());
         }
      } else {
         std::printf("unrecognized serviceName '%s'\n", serviceName.c_str());
      }
   }
   catch (const BasicException& be)
   {
      std::printf("BasicException caught: '%s'\n", be.whatString().c_str());
   }
   catch (const std::exception& e)
   {
      std::printf("exception caught: '%s'\n", e.what());
   }
   catch (...)
   {
      std::printf("unknown exception caught\n");
   }
}

//******************************************************************************
