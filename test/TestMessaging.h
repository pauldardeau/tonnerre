// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef TONNERRE_TESTMESSAGING_H
#define TONNERRE_TESTMESSAGING_H

#include "TestSuite.h"


namespace tonnerre {

class TestMessaging : public chaudiere::TestSuite {

protected:
   void runTests();

   void testSetMessaging();
   void testGetMessaging();
   void testInitialize();
   void testIsInitialized();

   void testConstructor();

   void testRegisterService();
   void testIsServiceRegistered();
   void testGetInfoForService();
   void testSocketForService();
   void testReturnSocketForService();

public:
   TestMessaging();

};

}

#endif

