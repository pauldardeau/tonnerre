// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef TONNERRE_TESTMESSAGINGSERVER_H
#define TONNERRE_TESTMESSAGINGSERVER_H

#include "TonnerreTestSuite.h"


namespace tonnerre {

class TestMessagingServer : public tonnerre_test::TonnerreTestSuite {

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

