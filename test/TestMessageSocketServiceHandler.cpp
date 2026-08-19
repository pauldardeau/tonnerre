// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#include "TestMessageSocketServiceHandler.h"
#include "MessageSocketServiceHandler.h"
#include "MessageHandler.h"
#include "Message.h"
#include "KeyValuePairs.h"
#include "SocketRequest.h"
#include "SocketServiceHandler.h"
#include "LoopbackConnection.h"

using namespace tonnerre;
using namespace chaudiere;

namespace {

// Echoes the request payload back as the response payload.
class EchoMessageHandler : public tonnerre::MessageHandler {
public:
   void handleTextMessage(const Message&,
                          Message&,
                          const std::string&,
                          const std::string& requestPayload,
                          std::string& responsePayload) override {
      responsePayload = requestPayload;
   }

   void handleKeyValuesMessage(const Message&,
                               Message&,
                               const std::string&,
                               const chaudiere::KeyValuePairs& requestPayload,
                               chaudiere::KeyValuePairs& responsePayload) override {
      responsePayload = requestPayload;
   }
};

// Unused by these tests directly, but required to construct a SocketRequest.
class NoOpSocketServiceHandler : public chaudiere::SocketServiceHandler {
public:
   void serviceSocket(chaudiere::SocketRequest*) override {
   }

   const std::string& getName() const override {
      static const std::string name = "NoOpSocketServiceHandler";
      return name;
   }
};

}

//******************************************************************************

TestMessageSocketServiceHandler::TestMessageSocketServiceHandler() :
   poivre::TestSuite("TestMessageSocketServiceHandler") {
}

//******************************************************************************

void TestMessageSocketServiceHandler::runTests() {
   testConstructor();
   testServiceSocket();
   testGetName();
}

//******************************************************************************

void TestMessageSocketServiceHandler::testConstructor() {
   TEST_CASE("testConstructor");

   EchoMessageHandler echoHandler;
   MessageSocketServiceHandler handler(&echoHandler);
   requireStringEquals("MessageSocketServiceHandler", handler.getName(), "constructed handler should report its name");
}

//******************************************************************************

void TestMessageSocketServiceHandler::testServiceSocket() {
   TEST_CASE("testServiceSocket");

   const int port = 34717;
   tonnerre_test::LoopbackConnection conn(port);

   Message request("serviceTest", MessageTypeKeyValues);
   KeyValuePairs requestPayload;
   requestPayload.addPair("x", "y");
   request.setKeyValuesPayload(requestPayload);
   require(conn.clientSocket->write(request.toString()), "writing request should succeed");

   NoOpSocketServiceHandler unusedHandler; // only needed to satisfy SocketRequest's constructor
   Socket* serverSocket = conn.serverSideSocket;
   conn.serverSideSocket = nullptr; // ownership transferred to the SocketRequest below
   SocketRequest* socketRequest = new SocketRequest(serverSocket, &unusedHandler);

   EchoMessageHandler echoHandler;
   MessageSocketServiceHandler serviceHandler(&echoHandler);
   serviceHandler.serviceSocket(socketRequest);

   Message response;
   require(response.reconstitute(conn.clientSocket), "client should be able to reconstitute the response");
   requireStringEquals("serviceTest", response.getRequestName(), "response should echo the request name");
   requireStringEquals("y", response.getKeyValuesPayload().getValue("x"), "response payload should contain the echoed value");

   // serviceSocket() doesn't take ownership of the SocketRequest; its
   // destructor deletes serverSocket in turn.
   delete socketRequest;
}

//******************************************************************************

void TestMessageSocketServiceHandler::testGetName() {
   TEST_CASE("testGetName");

   EchoMessageHandler echoHandler;
   MessageSocketServiceHandler handler(&echoHandler);
   requireStringEquals("MessageSocketServiceHandler", handler.getName(), "getName should return 'MessageSocketServiceHandler'");
}

//******************************************************************************
