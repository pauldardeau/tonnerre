// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#include <fstream>

#include "TestMessaging.h"
#include "Messaging.h"
#include "ServiceInfo.h"
#include "ServerSocket.h"
#include "Socket.h"
#include "InvalidKeyException.h"

using namespace tonnerre;
using namespace chaudiere;

//******************************************************************************

TestMessaging::TestMessaging() :
   poivre::TestSuite("TestMessaging") {
}

//******************************************************************************

void TestMessaging::runTests() {
   testSetMessaging();
   testGetMessaging();
   testInitialize();
   testIsInitialized();

   testConstructor();

   testRegisterService();
   testIsServiceRegistered();
   testGetInfoForService();
   testSocketForService();
   testReturnSocketForService();
}

//******************************************************************************

void TestMessaging::testSetMessaging() {
   TEST_CASE("testSetMessaging");

   Messaging::setMessaging(new Messaging());
   std::shared_ptr<Messaging> retrieved = Messaging::getMessaging();
   require(nullptr != retrieved, "getMessaging should return non-null after setMessaging");
}

//******************************************************************************

void TestMessaging::testGetMessaging() {
   TEST_CASE("testGetMessaging");

   Messaging* newInstance = new Messaging();
   Messaging::setMessaging(newInstance);

   std::shared_ptr<Messaging> retrieved = Messaging::getMessaging();
   require(retrieved.get() == newInstance, "getMessaging should return the instance passed to setMessaging");
}

//******************************************************************************

void TestMessaging::testInitialize() {
   TEST_CASE("testInitialize");

   const std::string configPath = getTempFile();
   std::ofstream configFile(configPath.c_str());
   configFile << "[services]\n";
   configFile << "init_test_service = InitTestService\n";
   configFile << "\n";
   configFile << "[InitTestService]\n";
   configFile << "host = 127.0.0.1\n";
   configFile << "port = 9200\n";
   configFile.close();

   Messaging::initialize(configPath);

   std::shared_ptr<Messaging> messaging = Messaging::getMessaging();
   require(nullptr != messaging, "initialize should establish a Messaging singleton");
   require(messaging->isServiceRegistered("init_test_service"), "initialize should register services listed in the config file");

   deleteFile(configPath);
}

//******************************************************************************

void TestMessaging::testIsInitialized() {
   TEST_CASE("testIsInitialized");

   Messaging::setMessaging(new Messaging());
   require(Messaging::isInitialized(), "isInitialized should be true after setMessaging with a non-null instance");
}

//******************************************************************************

void TestMessaging::testConstructor() {
   TEST_CASE("testConstructor");

   Messaging messaging;
   requireFalse(messaging.isServiceRegistered("anyService"), "a newly constructed Messaging should have no registered services");
}

//******************************************************************************

void TestMessaging::testRegisterService() {
   TEST_CASE("testRegisterService");

   Messaging messaging;
   ServiceInfo serviceInfo("myService", "127.0.0.1", 9100);
   messaging.registerService("myService", serviceInfo);

   require(messaging.isServiceRegistered("myService"), "service should be registered after registerService");
}

//******************************************************************************

void TestMessaging::testIsServiceRegistered() {
   TEST_CASE("testIsServiceRegistered");

   Messaging messaging;
   requireFalse(messaging.isServiceRegistered("notRegistered"), "unregistered service should report false");

   ServiceInfo serviceInfo("anotherService", "127.0.0.1", 9101);
   messaging.registerService("anotherService", serviceInfo);
   require(messaging.isServiceRegistered("anotherService"), "registered service should report true");
}

//******************************************************************************

void TestMessaging::testGetInfoForService() {
   TEST_CASE("testGetInfoForService");

   Messaging messaging;
   ServiceInfo serviceInfo("infoService", "192.168.1.50", 9102);
   messaging.registerService("infoService", serviceInfo);

   const ServiceInfo retrieved = messaging.getInfoForService("infoService");
   requireStringEquals("192.168.1.50", retrieved.host(), "retrieved host should match registered value");
   require(retrieved.port() == 9102, "retrieved port should match registered value");

   bool caughtException = false;
   try {
      messaging.getInfoForService("missingService");
   } catch (const chaudiere::InvalidKeyException&) {
      caughtException = true;
   }
   require(caughtException, "getInfoForService should throw InvalidKeyException for an unregistered service");
}

//******************************************************************************

void TestMessaging::testSocketForService() {
   TEST_CASE("testSocketForService");

   const int port = 34702;
   ServerSocket serverListener(port);

   Messaging messaging;
   ServiceInfo serviceInfo("socketTestService", "127.0.0.1", (unsigned short) port);

   Socket* socket = messaging.socketForService(serviceInfo);
   require(nullptr != socket, "socketForService should return a non-null socket on cache miss");
   require(socket->isConnected(), "socket returned by socketForService should be connected");

   Socket* acceptedSocket = serverListener.accept();

   delete socket;
   delete acceptedSocket;
}

//******************************************************************************

void TestMessaging::testReturnSocketForService() {
   TEST_CASE("testReturnSocketForService");

   const int port = 34703;
   ServerSocket serverListener(port);

   Messaging messaging;
   ServiceInfo serviceInfo("returnTestService", "127.0.0.1", (unsigned short) port);

   Socket* original = messaging.socketForService(serviceInfo);
   Socket* accepted = serverListener.accept();

   messaging.returnSocketForService(serviceInfo, original);

   // returned socket should now be served back out from the pool on the
   // next request for the same service, rather than opening a new connection
   Socket* pooled = messaging.socketForService(serviceInfo);
   require(pooled == original, "returnSocketForService should make the socket available for reuse");

   delete pooled;
   delete accepted;
}

//******************************************************************************
