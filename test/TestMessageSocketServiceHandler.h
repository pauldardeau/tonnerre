// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef TONNERRE_TESTMESSAGESOCKETSERVICEHANDLER_H
#define TONNERRE_TESTMESSAGESOCKETSERVICEHANDLER_H

#include "TestSuite.h"


namespace tonnerre {

class TestMessageSocketServiceHandler : public chaudiere::TestSuite {

protected:
   void runTests();

   void testConstructor();
   void testServiceSocket();
   void testGetName();

public:
   TestMessageSocketServiceHandler();

};

}

#endif

