// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#include <string>
#include <vector>
#include <stdio.h>

#include "Messaging.h"
#include "Message.h"
#include "KeyValuePairs.h"
#include "BasicException.h"
#include "Logger.h"
#include "StdLogger.h"

using namespace std;
using namespace tonnerre;
using namespace chaudiere;

//******************************************************************************

void PrintKeyValues(const KeyValuePairs& kvp) {
   vector<string> keys;
   kvp.getKeys(keys);
   const vector<string>::const_iterator itEnd = keys.end();
   vector<string>::const_iterator it = keys.begin();
   for (; it != itEnd; it++) {
      const string& key = *it;
      const string& value = kvp.getValue(key);
      ::printf("key='%s', value='%s'\n", key.c_str(), value.c_str());
   }
}

//******************************************************************************

int main(int argc, char* argv[])
{
   StdLogger* logger = new StdLogger(Info);
   //logger->setLogInstanceLifecycles(true);
   Logger::setLogger(logger);
   
   const string SERVICE_SERVER_INFO = "server_info";
   const string SERVICE_ECHO        = "echo_service";
   const string SERVICE_STOOGE_INFO = "stooge_info_service";

   string serviceName;
   //serviceName = SERVICE_SERVER_INFO;
   serviceName = SERVICE_ECHO;
   //serviceName = SERVICE_STOOGE_INFO;

   try {
      Messaging::initialize("tonnerre.ini");
   
      if (serviceName == SERVICE_SERVER_INFO) {
         Message message("serverInfo", MessageTypeText);
         Message response;
         if (message.send(serviceName, response)) {
            const string& responseText = response.getTextPayload();
            ::printf("response: '%s'\n", responseText.c_str());
         } else {
            ::printf("error: unable to send message to service '%s'\n", serviceName.c_str());
         }
      } else if (serviceName == SERVICE_ECHO) {
         KeyValuePairs kvp;
         kvp.addPair("firstName", "Mickey");
         kvp.addPair("lastName", "Mouse");
         kvp.addPair("city", "Orlando");
         kvp.addPair("state", "FL");

         Message message("echo", MessageTypeKeyValues);
         message.setKeyValuesPayload(kvp);
         Message response;
         if (message.send(serviceName, response)) {
            const KeyValuePairs& responseKeyValues = response.getKeyValuesPayload();
            PrintKeyValues(responseKeyValues);
         } else {
            ::printf("error: unable to send message to service '%s'\n", serviceName.c_str());
         }
      } else if (serviceName == SERVICE_STOOGE_INFO) {
         Message message("listStooges", MessageTypeKeyValues);
         Message response;
         if (message.send(serviceName, response)) {
            const KeyValuePairs& responseKeyValues = response.getKeyValuesPayload();
            PrintKeyValues(responseKeyValues);
         } else {
            ::printf("error: unable to send message to service '%s'\n", serviceName.c_str());
         }
      } else {
         ::printf("unrecognized serviceName '%s'\n", serviceName.c_str());
      }
   } catch (const BasicException& be) {
      ::printf("BasicException caught: '%s'\n", be.whatString().c_str());
   } catch (const exception& e) {
      ::printf("exception caught: '%s'\n", e.what());
   } catch (...) {
      ::printf("unknown exception caught\n");
   }
}

//******************************************************************************
