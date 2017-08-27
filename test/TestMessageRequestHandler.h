// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef TONNERRE_TESTMESSAGEREQUESTHANDLER_H
#define TONNERRE_TESTMESSAGEREQUESTHANDLER_H

#include "TestSuite.h"


namespace tonnerre {

class TestMessageRequestHandler : public poivre::TestSuite {

protected:
   void runTests();

   void testConstructorWithSocket();
   void testConstructorWithSocketRequest();   
   void testRun();

public:
   TestMessageRequestHandler();

};
   
}

#endif

