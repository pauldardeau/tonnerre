// Copyright Paul Dardeau 2017
// BSD License

#include <cstdio>
#include <cstring>
#include <exception>
#include <memory>
#include <string>

#include "TestSuite.h"
#include "TonnerreTestSuite.h"
#include "TestMessaging.h"
#include "TestMessagingServer.h"
#include "TestMessage.h"
#include "TestMessageRequestHandler.h"
#include "TestMessageSocketServiceHandler.h"
#ifndef _WIN32
#include "TestReceivePath.h"
#endif

using namespace tonnerre;
using namespace tonnerre_test;

//******************************************************************************

namespace {

struct RunTotals {
   int suitesRun = 0;
   int suitesEmpty = 0;        // suites that executed no assertions at all
   int suiteExceptions = 0;    // exceptions that escaped a suite
   int assertions = 0;
   int failures = 0;
};

RunTotals g_totals;

void runSuite(std::unique_ptr<poivre::TestSuite> suite) {
   const std::string name = suite->getName();
   const RunConfig& config = runConfig();

   if (!config.suiteFilter.empty() && (config.suiteFilter != name)) {
      return;
   }

   bool escaped = false;

   try {
      suite->run();
   } catch (const std::exception& e) {
      escaped = true;
      printf("*** failure: exception escaped suite %s: %s\n", name.c_str(), e.what());
   } catch (...) {
      escaped = true;
      printf("*** failure: unknown exception escaped suite %s\n", name.c_str());
   }
   fflush(stdout);

   ++g_totals.suitesRun;
   g_totals.assertions += suite->getNumTestsExecuted();
   g_totals.failures += suite->getNumFailures();

   if (escaped) {
      ++g_totals.suiteExceptions;
      ++g_totals.failures;
   }

   if (suite->getNumTestsExecuted() == 0) {
      ++g_totals.suitesEmpty;
      printf("*** failure: suite %s executed no assertions\n", name.c_str());
      fflush(stdout);
   }
}

void run_tests() {
   runSuite(std::unique_ptr<poivre::TestSuite>(new TestMessaging));
   runSuite(std::unique_ptr<poivre::TestSuite>(new TestMessagingServer));
   runSuite(std::unique_ptr<poivre::TestSuite>(new TestMessage));
   runSuite(std::unique_ptr<poivre::TestSuite>(new TestMessageRequestHandler));
   runSuite(std::unique_ptr<poivre::TestSuite>(new TestMessageSocketServiceHandler));
#ifndef _WIN32
   runSuite(std::unique_ptr<poivre::TestSuite>(new TestReceivePath));
#endif
}

void printUsage(const char* program) {
   printf("usage: %s [--group all|unit] [--suite <SuiteName>]\n", program);
   printf("  all    every suite and every isolated scenario (default)\n");
   printf("  unit   the same as all (there are no known defects to set apart any more)\n");
   printf("set TONNERRE_TEST_SLOW=1 to include the scenario that waits out the 10 s default deadline\n");
   printf("set TONNERRE_TEST_VERBOSE=1 to show what passing receive-path scenarios observed\n");
}

}

//******************************************************************************

int main(int argc, char* argv[]) {
   for (int i = 1; i < argc; ++i) {
      const std::string arg(argv[i]);
      if ((arg == "--group") && (i + 1 < argc)) {
         const std::string value(argv[++i]);
         if (value == "all") {
            runConfig().group = Group::All;
         } else if (value == "unit") {
            runConfig().group = Group::Unit;
         } else {
            printUsage(argv[0]);
            return 64;
         }
      } else if ((arg == "--suite") && (i + 1 < argc)) {
         runConfig().suiteFilter = argv[++i];
      } else {
         printUsage(argv[0]);
         return 64;
      }
   }

   int exitCode = 0;
   std::string reason;

   try {
      run_tests();
   } catch (const std::exception& e) {
      printf("*** failure: exception escaped the test runner: %s\n", e.what());
      ++g_totals.suiteExceptions;
      ++g_totals.failures;
   } catch (...) {
      printf("*** failure: unknown exception escaped the test runner\n");
      ++g_totals.suiteExceptions;
      ++g_totals.failures;
   }

   const int testCases = TonnerreTestSuite::testCasesStarted();
   const ScenarioTally& tally = scenarioTally();

   if ((g_totals.suitesRun == 0) || (testCases == 0) || (g_totals.assertions == 0)) {
      exitCode = 2;
      reason = "no tests were executed";
   } else if (g_totals.failures > 0) {
      exitCode = 1;
      reason = std::to_string(g_totals.failures) + " failed assertion(s)";
   } else if (g_totals.suitesEmpty > 0) {
      exitCode = 1;
      reason = "a suite executed no assertions";
   }

   const char* groupName = (runConfig().group == Group::Unit) ? "unit" : "all";

   printf("\n=============================================================\n");
   printf("Tonnerre test summary (group: %s%s%s)\n",
          groupName,
          runConfig().suiteFilter.empty() ? "" : ", suite: ",
          runConfig().suiteFilter.c_str());
   printf("  suites run:        %d\n", g_totals.suitesRun);
   printf("  test cases:        %d\n", testCases);
   printf("  assertions:        %d\n", g_totals.assertions);
   printf("  failed assertions: %d\n", g_totals.failures);
   printf("  suite exceptions:  %d\n", g_totals.suiteExceptions);
   if (tally.run > 0) {
      printf("  isolated receive-path scenarios (subprocess, loopback):\n");
      printf("    run:                       %d\n", tally.run);
      printf("    passed:                    %d\n", tally.passed);
      printf("    failed:                    %d\n", tally.unexpectedFailures);
      printf("    crashes / timeouts:        %d / %d\n", tally.crashes, tally.timeouts);
      printf("    sanitizer findings:        %d\n", tally.sanitizerFindings);
      printf("    escaped exceptions:        %d\n", tally.escapedExceptions);
   }
   printf("RESULT: %s%s%s\n",
          (exitCode == 0) ? "PASS" : "FAIL",
          reason.empty() ? "" : " - ",
          reason.c_str());
   printf("=============================================================\n");
   fflush(stdout);

   return exitCode;
}

//******************************************************************************
