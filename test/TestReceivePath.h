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
 * payload-size boundaries (D-3), stalled peers and the receive deadline (D-8),
 * and the response deadline of Message::send(service, response, deadline).
 *
 * Every scenario runs in a forked child process (see IsolatedCase.h) against
 * a real loopback TCP connection on a kernel-chosen port, so a crash, a hang
 * or a sanitizer report in the code under test cannot take the runner down.
 * Deadline scenarios pass sub-second deadlines explicitly; the 10 s production
 * default is not shortened for the tests.
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
   void testReceiveDeadline();
   void testResponseDeadline();
   void testDeadlineArguments();

   // runs one scenario in an isolated child process and records the outcome
   void runScenario(const ReceiveScenario& scenario);
};

}

#endif
