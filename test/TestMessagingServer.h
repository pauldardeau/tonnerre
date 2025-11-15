// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef TONNERRE_TESTMESSAGINGSERVER_H
#define TONNERRE_TESTMESSAGINGSERVER_H

#include "TestSuite.h"


namespace tonnerre {

class TestMessagingServer : public chaudiere::TestSuite {

protected:
   void runTests();

   void testConstructor();
   void testSetMessageHandler();
   void testHandlerForSocket();
   void testHandlerForSocketRequest();
   void testCreateSocketServiceHandler();

public:
   TestMessagingServer();

};

}

#endif

