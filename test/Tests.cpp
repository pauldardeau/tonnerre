// Copyright Paul Dardeau 2017
// BSD License

#include "TestSuite.h"
#include "TestMessaging.h"
#include "TestMessagingServer.h"
#include "TestMessage.h"
#include "TestMessageRequestHandler.h"
#include "TestMessageSocketServiceHandler.h"

using namespace tonnerre;

//******************************************************************************

void run_test(poivre::TestSuite* test_suite) {
   if (NULL != test_suite) {
      test_suite->run();
      delete test_suite;
   }
}

//******************************************************************************

void run_tests() {
   run_test(new TestMessaging);
   run_test(new TestMessagingServer);
   run_test(new TestMessage);
   run_test(new TestMessageRequestHandler);
   run_test(new TestMessageSocketServiceHandler);
}

//******************************************************************************

int main(int argc, char* argv[]) {
   run_tests();
}

//******************************************************************************

