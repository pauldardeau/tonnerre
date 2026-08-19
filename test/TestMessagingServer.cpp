// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#include <fstream>

#include "TestMessagingServer.h"
#include "MessagingServer.h"
#include "MessageHandler.h"
#include "Message.h"
#include "KeyValuePairs.h"
#include "RequestHandler.h"
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

void writeServerConfig(const std::string& configPath, int port) {
   std::ofstream configFile(configPath.c_str());
   configFile << "[server]\n";
   configFile << "port = " << port << "\n";
   configFile << "threading = none\n";
   configFile.close();
}

}

//******************************************************************************

TestMessagingServer::TestMessagingServer() :
   poivre::TestSuite("TestMessagingServer") {
}

//******************************************************************************

void TestMessagingServer::runTests() {
   testConstructor();
   testSetMessageHandler();
   testHandlerForSocket();
   testHandlerForSocketRequest();
   testCreateSocketServiceHandler();
}

//******************************************************************************

void TestMessagingServer::testConstructor() {
   TEST_CASE("testConstructor");

   const std::string configPath = getTempFile();
   writeServerConfig(configPath, 34710);

   MessagingServer server(configPath, "someService");
   require(true, "MessagingServer should construct successfully from a valid config file");

   deleteFile(configPath);
}

//******************************************************************************

void TestMessagingServer::testSetMessageHandler() {
   TEST_CASE("testSetMessageHandler");

   const std::string configPath = getTempFile();
   writeServerConfig(configPath, 34711);

   MessagingServer server(configPath, "someService");

   EchoMessageHandler echoHandler;
   server.setMessageHandler(&echoHandler);
   require(true, "setMessageHandler should not throw when given a valid handler");

   deleteFile(configPath);
}

//******************************************************************************

void TestMessagingServer::testHandlerForSocket() {
   TEST_CASE("testHandlerForSocket");

   const std::string configPath = getTempFile();
   writeServerConfig(configPath, 34712);

   MessagingServer server(configPath, "someService");

   EchoMessageHandler echoHandler;
   server.setMessageHandler(&echoHandler);

   const int connPort = 34713;
   tonnerre_test::LoopbackConnection conn(connPort);

   Message request("pingTest", MessageTypeText);
   request.setTextPayload("ping");
   require(conn.clientSocket->write(request.toString()), "writing request should succeed");

   Socket* serverSocket = conn.serverSideSocket;
   conn.serverSideSocket = nullptr; // ownership transferred to the RequestHandler below

   RequestHandler* requestHandler = server.handlerForSocket(serverSocket);
   require(nullptr != requestHandler, "handlerForSocket should return a non-null RequestHandler");
   require(requestHandler->getSocket() == serverSocket, "the RequestHandler should wrap the given socket");

   // exercises the handler set via setMessageHandler() end-to-end
   requestHandler->run();

   Message response;
   require(response.reconstitute(conn.clientSocket), "client should be able to reconstitute the response");
   requireStringEquals("ping", response.getTextPayload(), "response payload should be echoed back");

   delete requestHandler;
   deleteFile(configPath);
}

//******************************************************************************

void TestMessagingServer::testHandlerForSocketRequest() {
   TEST_CASE("testHandlerForSocketRequest");

   const std::string configPath = getTempFile();
   writeServerConfig(configPath, 34714);

   MessagingServer server(configPath, "someService");

   EchoMessageHandler echoHandler;
   server.setMessageHandler(&echoHandler);

   const int connPort = 34715;
   tonnerre_test::LoopbackConnection conn(connPort);

   Message request("pongTest", MessageTypeText);
   request.setTextPayload("pong");
   require(conn.clientSocket->write(request.toString()), "writing request should succeed");

   NoOpSocketServiceHandler socketServiceHandler;
   Socket* serverSocket = conn.serverSideSocket;
   conn.serverSideSocket = nullptr; // ownership transferred to the SocketRequest below
   SocketRequest* socketRequest = new SocketRequest(serverSocket, &socketServiceHandler);

   RequestHandler* requestHandler = server.handlerForSocketRequest(socketRequest);
   require(nullptr != requestHandler, "handlerForSocketRequest should return a non-null RequestHandler");

   requestHandler->run();

   Message response;
   require(response.reconstitute(conn.clientSocket), "client should be able to reconstitute the response");
   requireStringEquals("pong", response.getTextPayload(), "response payload should be echoed back");

   // MessageRequestHandler doesn't own the SocketRequest (isAutoDelete
   // defaults to false), so it's this test's responsibility to delete it;
   // SocketRequest's destructor deletes serverSocket in turn.
   delete requestHandler;
   delete socketRequest;

   deleteFile(configPath);
}

//******************************************************************************

void TestMessagingServer::testCreateSocketServiceHandler() {
   TEST_CASE("testCreateSocketServiceHandler");

   const std::string configPath = getTempFile();
   writeServerConfig(configPath, 34716);

   MessagingServer server(configPath, "someService");

   EchoMessageHandler echoHandler;
   server.setMessageHandler(&echoHandler);

   SocketServiceHandler* serviceHandler = server.createSocketServiceHandler();
   require(nullptr != serviceHandler, "createSocketServiceHandler should return a non-null handler");
   requireStringEquals("MessageSocketServiceHandler", serviceHandler->getName(), "handler name should identify it as a MessageSocketServiceHandler");

   delete serviceHandler;
   deleteFile(configPath);
}

//******************************************************************************
