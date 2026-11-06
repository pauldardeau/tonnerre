// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef TONNERRE_TESTMESSAGESOCKETSERVICEHANDLER_H
#define TONNERRE_TESTMESSAGESOCKETSERVICEHANDLER_H

#include "TonnerreTestSuite.h"


namespace tonnerre {

class TestMessageSocketServiceHandler : public tonnerre_test::TonnerreTestSuite {

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

