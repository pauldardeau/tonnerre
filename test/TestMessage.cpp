// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#include "TestMessage.h"
#include "Messaging.h"

using namespace tonnerre;

//******************************************************************************

TestMessage::TestMessage() :
   TestSuite("TestMessage") {
}

//******************************************************************************

void TestMessage::runTests() {
   testDefaultConstructor();
   testConstructor();
   testCopyConstructor();
   testSend();
   testSendWithMessage();
   testAssignmentOperator();
   testReconstitute();
   testSetType();
   testGetType();
   testGetRequestName();
   testGetKeyValuesPayload();
   testGetTextPayload();
   testSetKeyValuesPayload();
   testSetTextPayload();
   testGetServiceName();
   testToString();
   testToStringKVP();
   testFromString();
   testEncodeLength();
   testDecodeLength();
   testSocketForService();
   testSetHeader();
   testHasHeader();
   testGetHeader();
   testReadSocketBytes();
}

//******************************************************************************

void TestMessage::testDefaultConstructor() {
   //TEST_CASE("testDefaultConstructor");
   //TODO: implement testDefaultConstructor
}

//******************************************************************************

void TestMessage::testConstructor() {
   //TEST_CASE("testConstructor");
   //TODO: implement testConstructor
}

//******************************************************************************

void TestMessage::testCopyConstructor() {
   //TEST_CASE("testCopyConstructor");
   //TODO: implement testCopyConstructor
}

//******************************************************************************

void TestMessage::testSend() {
   //TEST_CASE("testSend");
   //TODO: implement testSend
}

//******************************************************************************

void TestMessage::testSendWithMessage() {
   //TEST_CASE("testSendWithMessage");
   //TODO: implement testSendWithMessage
}

//******************************************************************************

void TestMessage::testAssignmentOperator() {
   //TEST_CASE("testAssignmentOperator");
   //TODO: implement testAssignmentOperator
}

//******************************************************************************

void TestMessage::testReconstitute() {
   //TEST_CASE("testReconstitute");
   //TODO: implement testReconstitute
}

//******************************************************************************

void TestMessage::testSetType() {
   //TEST_CASE("testSetType");
   //TODO: implement testSetType
}

//******************************************************************************

void TestMessage::testGetType() {
   //TEST_CASE("testGetType");
   //TODO: implement testGetType
}

//******************************************************************************

void TestMessage::testGetRequestName() {
   //TEST_CASE("testGetRequestName");
   //TODO: implement testGetRequestName
}

//******************************************************************************

void TestMessage::testGetKeyValuesPayload() {
   //TEST_CASE("testGetKeyValuesPayload");
   //TODO: implement testGetKeyValuesPayload
}

//******************************************************************************

void TestMessage::testGetTextPayload() {
   //TEST_CASE("testGetTextPayload");
   //TODO: implement testGetTextPayload
}

//******************************************************************************

void TestMessage::testSetKeyValuesPayload() {
   //TEST_CASE("testSetKeyValuesPayload");
   //TODO: implement testSetKeyValuesPayload
}

//******************************************************************************

void TestMessage::testSetTextPayload() {
   //TEST_CASE("testSetTextPayload");
   //TODO: implement testSetTextPayload
}

//******************************************************************************

void TestMessage::testGetServiceName() {
   //TEST_CASE("testGetServiceName");
   //TODO: implement testGetServiceName
}

//******************************************************************************

void TestMessage::testToString() {
   //TEST_CASE("testToString");
   //TODO: implement testToString
}

//******************************************************************************

void TestMessage::testToStringKVP() {
   //TEST_CASE("testToStringKVP");
   //TODO: implement testToStringKVP
}

//******************************************************************************

void TestMessage::testFromString() {
   //TEST_CASE("testFromString");
   //TODO: implement testFromString
}

//******************************************************************************

void TestMessage::testEncodeLength() {
   //TEST_CASE("testEncodeLength");
   //TODO: implement testEncodeLength
}

//******************************************************************************

void TestMessage::testDecodeLength() {
   //TEST_CASE("testDecodeLength");
   //TODO: implement testDecodeLength
}

//******************************************************************************

void TestMessage::testSocketForService() {
   //TEST_CASE("testSocketForService");
   //TODO: implement testSocketForService
}

//******************************************************************************

void TestMessage::testSetHeader() {
   //TEST_CASE("testSetHeader");
   //TODO: implement testSetHeader
}

//******************************************************************************

void TestMessage::testHasHeader() {
   //TEST_CASE("testHasHeader");
   //TODO: implement testHasHeader
}

//******************************************************************************

void TestMessage::testGetHeader() {
   //TEST_CASE("testGetHeader");
   //TODO: implement testGetHeader
}

//******************************************************************************

void TestMessage::testReadSocketBytes() {
   //TEST_CASE("testReadSocketBytes");
   //TODO: implement testReadSocketBytes
}

//******************************************************************************

