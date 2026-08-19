// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#include "TestMessage.h"
#include "Message.h"
#include "Messaging.h"
#include "KeyValuePairs.h"
#include "StrUtils.h"
#include "LoopbackConnection.h"

using namespace tonnerre;
using namespace chaudiere;

//******************************************************************************

TestMessage::TestMessage() :
   poivre::TestSuite("TestMessage") {
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
   TEST_CASE("testDefaultConstructor");

   Message message;
   require(message.getType() == MessageTypeUnknown, "default type should be MessageTypeUnknown");
   require(message.getRequestName().empty(), "default request name should be empty");
}

//******************************************************************************

void TestMessage::testConstructor() {
   TEST_CASE("testConstructor");

   Message message("doStuff", MessageTypeText);
   requireStringEquals("doStuff", message.getRequestName(), "request name should match constructor argument");
   require(message.getType() == MessageTypeText, "type should match constructor argument");
}

//******************************************************************************

void TestMessage::testCopyConstructor() {
   TEST_CASE("testCopyConstructor");

   Message original("copyRequest", MessageTypeKeyValues);
   original.setHeader("customHeader", "customValue");

   KeyValuePairs kvp;
   kvp.addPair("k", "v");
   original.setKeyValuesPayload(kvp);

   Message copy(original);

   requireStringEquals(original.getRequestName(), copy.getRequestName(), "copy should have same request name");
   require(copy.getType() == MessageTypeKeyValues, "copy should have same message type");
   require(copy.hasHeader("customHeader"), "copy should have same custom header");
   requireStringEquals("customValue", copy.getHeader("customHeader"), "copy header value");
   requireStringEquals("v", copy.getKeyValuesPayload().getValue("k"), "copy kvp payload");
}

//******************************************************************************

void TestMessage::testSend() {
   //TEST_CASE("testSend");
   //TODO: implement testSend (needs a registered Messaging service; see TestClient.cpp/TestServer.cpp for a full, real example)
}

//******************************************************************************

void TestMessage::testSendWithMessage() {
   //TEST_CASE("testSendWithMessage");
   //TODO: implement testSendWithMessage (needs a registered Messaging service; see TestClient.cpp/TestServer.cpp for a full, real example)
}

//******************************************************************************

void TestMessage::testAssignmentOperator() {
   TEST_CASE("testAssignmentOperator");

   Message source("assignRequest", MessageTypeText);
   source.setTextPayload("assigned text");

   Message target;
   target = source;

   requireStringEquals(source.getRequestName(), target.getRequestName(), "target should have same request name after assignment");
   require(target.getType() == MessageTypeText, "target should have same type after assignment");
   requireStringEquals("assigned text", target.getTextPayload(), "target should have same text payload after assignment");

   // self-assignment should be a safe no-op
   target = target;
   requireStringEquals("assigned text", target.getTextPayload(), "self-assignment should not corrupt state");
}

//******************************************************************************

void TestMessage::testReconstitute() {
   //TEST_CASE("testReconstitute");
   //TODO: implement testReconstitute
}

//******************************************************************************

void TestMessage::testSetType() {
   TEST_CASE("testSetType");

   Message message;
   message.setType(MessageTypeKeyValues);
   require(message.getType() == MessageTypeKeyValues, "getType should reflect setType");
}

//******************************************************************************

void TestMessage::testGetType() {
   TEST_CASE("testGetType");

   Message message("req", MessageTypeText);
   require(message.getType() == MessageTypeText, "getType should return the constructor-specified type");
}

//******************************************************************************

void TestMessage::testGetRequestName() {
   TEST_CASE("testGetRequestName");

   Message withName("namedRequest", MessageTypeText);
   requireStringEquals("namedRequest", withName.getRequestName(), "getRequestName should return the constructor-specified name");

   Message withoutName;
   require(withoutName.getRequestName().empty(), "getRequestName should be empty when not set");
}

//******************************************************************************

void TestMessage::testGetKeyValuesPayload() {
   TEST_CASE("testGetKeyValuesPayload");

   Message message;
   require(message.getKeyValuesPayload().empty(), "key/values payload should be empty by default");
}

//******************************************************************************

void TestMessage::testGetTextPayload() {
   TEST_CASE("testGetTextPayload");

   Message message;
   require(message.getTextPayload().empty(), "text payload should be empty by default");
}

//******************************************************************************

void TestMessage::testSetKeyValuesPayload() {
   TEST_CASE("testSetKeyValuesPayload");

   Message message;
   KeyValuePairs kvp;
   kvp.addPair("a", "1");
   kvp.addPair("b", "2");
   message.setKeyValuesPayload(kvp);

   requireStringEquals("1", message.getKeyValuesPayload().getValue("a"), "kvp payload 'a'");
   requireStringEquals("2", message.getKeyValuesPayload().getValue("b"), "kvp payload 'b'");
}

//******************************************************************************

void TestMessage::testSetTextPayload() {
   TEST_CASE("testSetTextPayload");

   Message message;
   message.setTextPayload("some text");
   requireStringEquals("some text", message.getTextPayload(), "text payload should match what was set");
}

//******************************************************************************

void TestMessage::testGetServiceName() {
   //TEST_CASE("testGetServiceName");
   //TODO: implement testGetServiceName (m_serviceName currently has no
   // setter and is never populated other than by copy/assignment - only
   // meaningful once a setter or reconstitute()-based population exists)
}

//******************************************************************************

void TestMessage::testToString() {
   TEST_CASE("testToString");

   Message message("myRequest", MessageTypeText);
   message.setTextPayload("hello world");

   const std::string wireFormat = message.toString();

   require(wireFormat.length() >= 10, "wire format should be at least the header-length-prefix size");

   std::string lengthPrefix = wireFormat.substr(0, 10);
   StrUtils::stripTrailing(lengthPrefix, ' ');
   const int headerLength = StrUtils::parseInt(lengthPrefix);

   require(headerLength > 0, "encoded header length should be positive");
   require((10 + (std::size_t) headerLength) <= wireFormat.length(),
           "wire format should be at least as long as prefix + declared header length");

   const std::string headersPortion = wireFormat.substr(10, headerLength);
   require(StrUtils::containsString(headersPortion, "request=myRequest"), "headers should contain the request name");

   const std::string payloadPortion = wireFormat.substr(10 + headerLength);
   requireStringEquals("hello world", payloadPortion, "payload portion should be the text payload");
}

//******************************************************************************

void TestMessage::testToStringKVP() {
   TEST_CASE("testToStringKVP");

   KeyValuePairs kvp;
   kvp.addPair("alpha", "one");
   kvp.addPair("beta", "two");

   const std::string asString = Message::toString(kvp);

   require(StrUtils::containsString(asString, "alpha=one"), "should contain alpha=one");
   require(StrUtils::containsString(asString, "beta=two"), "should contain beta=two");
   require(StrUtils::containsString(asString, ";"), "pairs should be delimited with ';'");
}

//******************************************************************************

void TestMessage::testFromString() {
   TEST_CASE("testFromString");

   KeyValuePairs kvp;
   const bool ok = Message::fromString("alpha=one;beta=two", kvp);

   require(ok, "fromString should report success for non-empty input");
   require(kvp.hasKey("alpha"), "should have parsed key 'alpha'");
   requireStringEquals("one", kvp.getValue("alpha"), "alpha value");
   require(kvp.hasKey("beta"), "should have parsed key 'beta'");
   requireStringEquals("two", kvp.getValue("beta"), "beta value");

   // round trip: toString(kvp) -> fromString(...) should reproduce the
   // same key/value pairs
   const std::string flattened = Message::toString(kvp);
   KeyValuePairs roundTripped;
   require(Message::fromString(flattened, roundTripped), "round-trip fromString should succeed");
   requireStringEquals("one", roundTripped.getValue("alpha"), "round-trip alpha value");
   requireStringEquals("two", roundTripped.getValue("beta"), "round-trip beta value");
}

//******************************************************************************

void TestMessage::testEncodeLength() {
   TEST_CASE("testEncodeLength");

   requireStringEquals("0", Message::encodeLength(0), "encodeLength(0)");
   requireStringEquals("42", Message::encodeLength(42), "encodeLength(42)");
   requireStringEquals("12345", Message::encodeLength(12345), "encodeLength(12345)");
}

//******************************************************************************

void TestMessage::testDecodeLength() {
   TEST_CASE("testDecodeLength");

   tonnerre_test::LoopbackConnection conn(34701);

   std::string lengthPrefix = Message::encodeLength(789);
   StrUtils::padRight(lengthPrefix, ' ', 10);
   require(conn.clientSocket->write(lengthPrefix), "writing length prefix should succeed");

   const std::size_t decoded = Message::decodeLength(conn.serverSideSocket);
   require(decoded == 789, "decoded length should match what was encoded");
}

//******************************************************************************

void TestMessage::testSocketForService() {
   //TEST_CASE("testSocketForService");
   //TODO: implement testSocketForService (needs a registered Messaging service)
}

//******************************************************************************

void TestMessage::testSetHeader() {
   TEST_CASE("testSetHeader");

   Message message;
   message.setHeader("customKey", "customValue");
   require(message.hasHeader("customKey"), "header should now be present");
}

//******************************************************************************

void TestMessage::testHasHeader() {
   TEST_CASE("testHasHeader");

   Message message;
   requireFalse(message.hasHeader("missingKey"), "header should not be present before being set");
   message.setHeader("presentKey", "value");
   require(message.hasHeader("presentKey"), "header should be present after being set");
}

//******************************************************************************

void TestMessage::testGetHeader() {
   TEST_CASE("testGetHeader");

   Message message;
   message.setHeader("greeting", "hello");
   requireStringEquals("hello", message.getHeader("greeting"), "getHeader should return the set value");
}

//******************************************************************************

void TestMessage::testReadSocketBytes() {
   //TEST_CASE("testReadSocketBytes");
   //TODO: implement testReadSocketBytes
}

//******************************************************************************
