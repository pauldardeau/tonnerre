// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#include "TestMessageRequestHandler.h"
#include "MessageRequestHandler.h"
#include "MessageHandler.h"
#include "Message.h"
#include "KeyValuePairs.h"
#include "SocketRequest.h"
#include "SocketServiceHandler.h"
#include "LoopbackConnection.h"

using namespace tonnerre;
using namespace chaudiere;

namespace {

// Echoes the request payload back as the response payload, so testRun can
// verify the full request -> handler -> response round trip.
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

TestMessageRequestHandler::TestMessageRequestHandler() :
   poivre::TestSuite("TestMessageRequestHandler") {
}

//******************************************************************************

void TestMessageRequestHandler::runTests() {
   testConstructorWithSocket();
   testConstructorWithSocketRequest();
   testRun();
}

//******************************************************************************

void TestMessageRequestHandler::testConstructorWithSocket() {
   TEST_CASE("testConstructorWithSocket");

   const int port = 34705;
   tonnerre_test::LoopbackConnection conn(port);

   EchoMessageHandler echoHandler;
   Socket* serverSocket = conn.serverSideSocket;
   conn.serverSideSocket = nullptr; // ownership transferred to the handler below

   MessageRequestHandler handler(serverSocket, &echoHandler);
   require(handler.getSocket() == serverSocket, "getSocket should return the socket passed to the constructor");
   require(handler.isSocketOwned(), "socket should be owned by the handler by default");
}

//******************************************************************************

void TestMessageRequestHandler::testConstructorWithSocketRequest() {
   TEST_CASE("testConstructorWithSocketRequest");

   const int port = 34706;
   tonnerre_test::LoopbackConnection conn(port);

   NoOpSocketServiceHandler socketServiceHandler;
   Socket* serverSocket = conn.serverSideSocket;
   conn.serverSideSocket = nullptr; // ownership transferred to the SocketRequest below

   SocketRequest* socketRequest = new SocketRequest(serverSocket, &socketServiceHandler);

   EchoMessageHandler echoHandler;
   {
      MessageRequestHandler handler(socketRequest, &echoHandler);
      require(handler.getSocket() == serverSocket, "getSocket should return the socket wrapped by the SocketRequest");
   }

   // MessageRequestHandler doesn't own the SocketRequest (isAutoDelete
   // defaults to false), so it's this test's responsibility to delete it;
   // SocketRequest's destructor deletes serverSocket in turn.
   delete socketRequest;
}

//******************************************************************************

void TestMessageRequestHandler::testRun() {
   TEST_CASE("testRun");

   const int port = 34707;
   tonnerre_test::LoopbackConnection conn(port);

   Message request("echoTest", MessageTypeKeyValues);
   KeyValuePairs requestPayload;
   requestPayload.addPair("greeting", "hello");
   request.setKeyValuesPayload(requestPayload);

   require(conn.clientSocket->write(request.toString()), "writing request to client socket should succeed");

   EchoMessageHandler echoHandler;
   Socket* serverSocket = conn.serverSideSocket;
   conn.serverSideSocket = nullptr; // ownership transferred to the handler below

   MessageRequestHandler handler(serverSocket, &echoHandler);
   handler.run();

   Message response;
   require(response.reconstitute(conn.clientSocket), "client should be able to reconstitute the response message");
   requireStringEquals("echoTest", response.getRequestName(), "response should echo the request name");
   requireStringEquals("hello", response.getKeyValuesPayload().getValue("greeting"), "response payload should contain the echoed value");
}

//******************************************************************************
