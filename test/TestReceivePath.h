// Copyright Paul Dardeau, SwampBits LLC 2026
// BSD License

#ifndef TONNERRE_TESTRECEIVEPATH_H
#define TONNERRE_TESTRECEIVEPATH_H

#include "TonnerreTestSuite.h"

namespace tonnerre {

struct ReceiveScenario;

/**
 * Receive-path regression coverage for tonnerre::Message::reconstitute():
 * malformed length prefixes (T0 defect D-1/D-4), truncated messages (D-2),
 * payload-size boundaries (D-3) and stalled peers (D-8).
 *
 * Every scenario runs in a forked child process (see IsolatedCase.h) against
 * a real loopback TCP connection on a kernel-chosen port, so a crash, a hang
 * or a sanitizer report in the code under test cannot take the runner down.
 * The expectations describe the *desired* behavior; scenarios that currently
 * fail are annotated with the defect they expose (see knownDefectFor() in
 * TestReceivePath.cpp) rather than weakened.
 */
class TestReceivePath : public tonnerre_test::TonnerreTestSuite {
public:
   TestReceivePath();

protected:
   void runTests() override;

   void testWellFormedMessages();
   void testMalformedHeaderLengthPrefix();
   void testMalformedPayloadLength();
   void testMalformedHeaders();
   void testTruncatedAtEveryByteOffset();
   void testPeerResetDuringMessage();
   void testPayloadSizeBoundaries();
   void testStalledPeer();

   // runs one scenario in an isolated child process and records the outcome
   void runScenario(const ReceiveScenario& scenario);
};

}

#endif
