// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef TONNERRE_TESTMESSAGE_H
#define TONNERRE_TESTMESSAGE_H

#include "TestSuite.h"


namespace tonnerre {

class TestMessage : public chaudiere::TestSuite {

protected:
   void runTests();

   void testDefaultConstructor();
   void testConstructor();
   void testCopyConstructor();
   void testSend();
   void testSendWithMessage();
   void testAssignmentOperator();
   void testReconstitute();
   void testSetType();
   void testGetType();
   void testGetRequestName();
   void testGetKeyValuesPayload();
   void testGetTextPayload();
   void testSetKeyValuesPayload();
   void testSetTextPayload();
   void testGetServiceName();
   void testToString();
   void testToStringKVP();
   void testFromString();
   void testEncodeLength();
   void testDecodeLength();
   void testSocketForService();
   void testSetHeader();
   void testHasHeader();
   void testGetHeader();
   void testReadSocketBytes();

public:
   TestMessage();

};

}

#endif

