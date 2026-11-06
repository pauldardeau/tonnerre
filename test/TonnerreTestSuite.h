// Copyright Paul Dardeau, SwampBits LLC 2026
// BSD License

#ifndef TONNERRE_TEST_TONNERRETESTSUITE_H
#define TONNERRE_TEST_TONNERRETESTSUITE_H

#include <string>

#include "TestSuite.h"

namespace tonnerre_test {

/**
 * Which part of the test run to execute (set by the runner from --group).
 *   All           everything (the default)
 *   Unit          the established suites plus every isolated scenario that is
 *                 not annotated with a known defect
 *   KnownDefects  only the isolated scenarios annotated with a known defect
 */
enum class Group {
   All,
   Unit,
   KnownDefects
};

struct RunConfig {
   Group group = Group::All;
   std::string suiteFilter;    // empty: every suite
};

inline RunConfig& runConfig() {
   static RunConfig config;
   return config;
}

/**
 * Totals for the isolated (subprocess) receive-path scenarios, printed in the
 * runner's summary. The pinned Poivre only counts assertions, so these are
 * kept here.
 */
struct ScenarioTally {
   int run = 0;
   int passed = 0;
   int knownDefectFailures = 0;      // failed, and annotated with a known defect
   int unexpectedFailures = 0;       // failed, and not annotated
   int knownDefectNowPassing = 0;    // annotated with a defect, but passed
   int crashes = 0;                  // child killed by a signal
   int timeouts = 0;                 // child exceeded its hard timeout
   int sanitizerFindings = 0;        // sanitizer report found in the child log
   int escapedExceptions = 0;        // exception escaped from the code under test
};

inline ScenarioTally& scenarioTally() {
   static ScenarioTally tally;
   return tally;
}

/**
 * Base class of every Tonnerre suite. Poivre (as pinned) counts assertions
 * and failures per suite but not test cases, and its test-case start hook is
 * not virtual; setup() runs once per test case, so it is used to count them.
 */
class TonnerreTestSuite : public poivre::TestSuite {
public:
   explicit TonnerreTestSuite(const std::string& suiteName) :
      poivre::TestSuite(suiteName) {
   }

   static int testCasesStarted() {
      return counter();
   }

protected:
   void setup() override {
      ++counter();
   }

private:
   static int& counter() {
      static int numTestCases = 0;
      return numTestCases;
   }
};

}

#endif
