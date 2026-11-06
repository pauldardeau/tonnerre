// Copyright Paul Dardeau, SwampBits LLC 2026
// BSD License

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <sys/socket.h>
#include <unistd.h>

#include "TestReceivePath.h"
#include "IsolatedCase.h"
#include "LoopbackPair.h"
#include "Message.h"
#include "KeyValuePairs.h"
#include "BasicException.h"
#include "Socket.h"

using namespace tonnerre;
using namespace tonnerre_test;

namespace tonnerre {

//******************************************************************************
// Scenario description
//******************************************************************************

// what the raw peer does after it has sent its bytes
enum class PeerFinish {
   Close,   // orderly shutdown (FIN) and close
   Reset,   // abortive close (RST)
   Hold     // keep the connection open until the receiver is done (a stalled peer)
};

// what the receiving side must do
enum class Expect {
   // reconstitute() must return false; no exception, crash, hang or sanitizer report
   Reject,
   // reconstitute() must return true and deliver exactly the expected message
   Accept,
   // either reject, or accept *exactly* the expected message. Never "true with
   // a payload that was silently altered or discarded". Used where the
   // supported maximum is a policy decision that T1b has to make.
   RejectOrAcceptExactly
};

struct ReceiveScenario {
   std::string id;
   std::string description;
   std::vector<std::string> segments;    // bytes the peer sends, one send() per segment
   int delayMillis = 0;                  // pause between segments
   PeerFinish finish = PeerFinish::Close;
   Expect expect = Expect::Reject;
   MessageType expectedType = MessageTypeText;
   std::string expectedText;
   std::vector<std::pair<std::string, std::string>> expectedPairs;
   int timeoutMillis = 8000;
   std::string defect;                   // known defect this scenario exposes ("" if none)
};

}

//******************************************************************************
// Known defects (T0 report, section 9; D-15 is new in T1a). A scenario listed
// here fails today and is reported as a known-defect scenario: it is expected
// to fail until the defect is fixed, and it still counts as a failure in the
// run. The list is the exact set of scenarios observed failing in T1a, tagged
// with the defect that explains the failure; it is never used to skip a
// scenario or to relax an expectation. T1b removes an entry when it fixes the
// defect behind it.
//
//   D-1   negative or wrapped length reaches `stackBuffer[numberBytes]`
//         (Message.cpp:290): out-of-bounds stack write. A crash or a sanitizer
//         report; in an unsanitized build the write is silent.
//   D-2   a failed or short readSocket() is treated as success, so truncated
//         messages are accepted and an early disconnect throws
//         (Message.cpp:289, 302, 328)
//   D-3   payloads over 32,767 bytes are dropped silently while
//         reconstitute() still returns true (Message.cpp:299-316, 374)
//   D-4   size_t lengths narrowed to int (Message.cpp:334-339, 367-372): a
//         length of 2^32+n is read as n
//   D-8   no receive timeout: a stalled peer blocks the receiver forever
//   D-15  non-numeric, empty or out-of-range numeric header text makes
//         StrUtils::parseLong throw out of reconstitute() (Message.cpp:334, 367)
//******************************************************************************

namespace {

struct DefectNote {
   const char* id;
   const char* defect;
};

const DefectNote KNOWN_DEFECTS[] = {
   { "header-length.decimal-point", "D-15" },
   { "header-length.digits-then-letters", "D-15" },
   { "header-length.embedded-nuls", "D-15" },
   { "header-length.empty-all-spaces", "D-15" },
   { "header-length.exponent", "D-15" },
   { "header-length.hex", "D-15" },
   { "header-length.int-max-plus-1", "D-1" },
   { "header-length.int-max-plus-2", "D-1" },
   { "header-length.int-min", "D-1" },
   { "header-length.leading-space", "D-15" },
   { "header-length.negative-1", "D-1" },
   { "header-length.negative-10-digits", "D-1" },
   { "header-length.negative-5", "D-1" },
   { "header-length.nonnumeric", "D-15" },
   { "header-length.plus-sign", "D-15" },
   { "header-length.uint-max", "D-1" },
   { "header-length.with-trailing-data.-1", "D-1" },
   { "header-length.with-trailing-data.2147483648", "D-1" },
   { "header-length.with-trailing-data.4294967295", "D-1" },
   { "header-length.wraps-to-100-with-valid-100-byte-header", "D-4" },
   { "header-length.wraps-to-50-with-valid-50-byte-header", "D-4" },
   { "payload-length.decimal", "D-15" },
   { "payload-length.exponent", "D-15" },
   { "payload-length.int-max", "D-3" },
   { "payload-length.int-max-plus-1", "D-1" },
   { "payload-length.leading-space", "D-15" },
   { "payload-length.negative-1", "D-1" },
   { "payload-length.nineteen-nines", "D-15" },
   { "payload-length.nonnumeric", "D-15" },
   { "payload-length.twenty-nines", "D-15" },
   { "payload-length.uint-max", "D-1" },
   { "payload-length.wraps-to-5-with-5-byte-payload", "D-4" },
   { "payload-size.declared-forty-thousand-sent-10", "D-3" },
   { "payload-size.declared-int-max-sent-10", "D-3" },
   { "payload-size.declared-one-hundred-million-sent-10", "D-3" },
   { "payload-size.kvp-3000-pairs", "D-3" },
   { "payload-size.text-100000", "D-3" },
   { "payload-size.text-32768", "D-3" },
   { "payload-size.text-40000", "D-3" },
   { "reset.end-of-header", "D-2" },
   { "reset.inside-payload", "D-2" },
   { "reset.no-bytes", "D-2" },
   { "stalled.silent-after-connect", "D-8" },
   { "stalled.silent-before-payload", "D-8" },
   { "stalled.silent-inside-prefix", "D-8" },
   { "truncate.kvp.header@43of70", "D-2" },
   { "truncate.kvp.header@44of70", "D-2" },
   { "truncate.kvp.header@45of70", "D-2" },
   { "truncate.kvp.header@46of70", "D-2" },
   { "truncate.kvp.header@47of70", "D-2" },
   { "truncate.kvp.header@48of70", "D-2" },
   { "truncate.kvp.header@49of70", "D-2" },
   { "truncate.kvp.header@50of70", "D-2" },
   { "truncate.kvp.header@51of70", "D-2" },
   { "truncate.kvp.header@52of70", "D-2" },
   { "truncate.kvp.header@53of70", "D-2" },
   { "truncate.kvp.header@54of70", "D-2" },
   { "truncate.kvp.header@55of70", "D-2" },
   { "truncate.kvp.header@56of70", "D-2" },
   { "truncate.kvp.header@57of70", "D-2" },
   { "truncate.kvp.header@58of70", "D-2" },
   { "truncate.kvp.header@59of70", "D-2" },
   { "truncate.kvp.header@60of70", "D-2" },
   { "truncate.kvp.payload@61of70", "D-2" },
   { "truncate.kvp.payload@62of70", "D-2" },
   { "truncate.kvp.payload@63of70", "D-2" },
   { "truncate.kvp.payload@64of70", "D-2" },
   { "truncate.kvp.payload@65of70", "D-2" },
   { "truncate.kvp.payload@66of70", "D-2" },
   { "truncate.kvp.payload@67of70", "D-2" },
   { "truncate.kvp.payload@68of70", "D-2" },
   { "truncate.kvp.payload@69of70", "D-2" },
   { "truncate.kvp.prefix@0of70", "D-2" },
   { "truncate.text.header@45of74", "D-2" },
   { "truncate.text.header@46of74", "D-2" },
   { "truncate.text.header@47of74", "D-2" },
   { "truncate.text.header@48of74", "D-2" },
   { "truncate.text.header@49of74", "D-2" },
   { "truncate.text.header@50of74", "D-2" },
   { "truncate.text.header@51of74", "D-2" },
   { "truncate.text.header@52of74", "D-2" },
   { "truncate.text.header@53of74", "D-2" },
   { "truncate.text.header@54of74", "D-2" },
   { "truncate.text.header@55of74", "D-2" },
   { "truncate.text.header@56of74", "D-2" },
   { "truncate.text.header@57of74", "D-2" },
   { "truncate.text.header@58of74", "D-2" },
   { "truncate.text.header@59of74", "D-2" },
   { "truncate.text.header@60of74", "D-2" },
   { "truncate.text.header@61of74", "D-2" },
   { "truncate.text.header@62of74", "D-2" },
   { "truncate.text.payload@63of74", "D-2" },
   { "truncate.text.payload@64of74", "D-2" },
   { "truncate.text.payload@65of74", "D-2" },
   { "truncate.text.payload@66of74", "D-2" },
   { "truncate.text.payload@67of74", "D-2" },
   { "truncate.text.payload@68of74", "D-2" },
   { "truncate.text.payload@69of74", "D-2" },
   { "truncate.text.payload@70of74", "D-2" },
   { "truncate.text.payload@71of74", "D-2" },
   { "truncate.text.payload@72of74", "D-2" },
   { "truncate.text.payload@73of74", "D-2" },
   { "truncate.text.prefix@0of74", "D-2" },
};

std::string knownDefectFor(const std::string& id) {
   for (const DefectNote& note : KNOWN_DEFECTS) {
      if (id == note.id) {
         return note.defect;
      }
   }
   return std::string();
}

//******************************************************************************
// Wire-format helpers (mirror the documented format; not production code)
//******************************************************************************

std::string pad10(std::string s) {
   while (s.size() < 10) {
      s += ' ';
   }
   return s;
}

// a complete frame: 10-char length prefix + header + payload
std::string frame(const std::string& header, const std::string& payload) {
   return pad10(std::to_string(header.size())) + header + payload;
}

std::string textHeader(const std::string& payloadLength,
                       const std::string& request = "myRequest") {
   return "payload_type=text;payload_length=" + payloadLength +
          ";request=" + request;
}

// a valid message produced by the production encoder
std::string validTextWire(const std::string& text,
                          const std::string& request = "myRequest") {
   Message message(request, MessageTypeText);
   message.setTextPayload(text);
   return message.toString();
}

std::string validKvpWire(const chaudiere::KeyValuePairs& kvp,
                         const std::string& request = "myRequest") {
   Message message(request, MessageTypeKeyValues);
   message.setKeyValuesPayload(kvp);
   return message.toString();
}

std::string repeatedChar(char c, std::size_t count) {
   return std::string(count, c);
}

//******************************************************************************
// The raw peer: sends the scripted bytes from its own thread
//******************************************************************************

class RawPeer {
public:
   RawPeer(int fd,
           const std::vector<std::string>& segments,
           int delayMillis,
           PeerFinish finish) :
      m_fd(fd),
      m_segments(segments),
      m_delayMillis(delayMillis),
      m_finish(finish),
      m_release(false) {
   }

   void start() {
      m_thread = std::thread([this]() { run(); });
   }

   // lets a PeerFinish::Hold peer go, then waits for the thread
   void releaseAndJoin() {
      m_release = true;
      if (m_thread.joinable()) {
         m_thread.join();
      }
   }

private:
   static bool sendAll(int fd, const std::string& bytes) {
      std::size_t sent = 0;
      while (sent < bytes.size()) {
#ifdef MSG_NOSIGNAL
         const ssize_t rc = ::send(fd, bytes.data() + sent, bytes.size() - sent, MSG_NOSIGNAL);
#else
         const ssize_t rc = ::send(fd, bytes.data() + sent, bytes.size() - sent, 0);
#endif
         if (rc <= 0) {
            return false;
         }
         sent += static_cast<std::size_t>(rc);
      }
      return true;
   }

   void run() {
      bool first = true;
      for (const std::string& segment : m_segments) {
         if (!first && (m_delayMillis > 0)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(m_delayMillis));
         }
         first = false;
         if (!sendAll(m_fd, segment)) {
            break;    // the receiver already closed: nothing more to deliver
         }
      }

      if (m_finish == PeerFinish::Hold) {
         while (!m_release) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
         }
      } else if (m_finish == PeerFinish::Reset) {
         struct linger lg;
         lg.l_onoff = 1;
         lg.l_linger = 0;
         ::setsockopt(m_fd, SOL_SOCKET, SO_LINGER, &lg, sizeof(lg));
      } else {
         ::shutdown(m_fd, SHUT_WR);
      }

      ::close(m_fd);
      m_fd = -1;
   }

   int m_fd;
   std::vector<std::string> m_segments;
   int m_delayMillis;
   PeerFinish m_finish;
   std::atomic<bool> m_release;
   std::thread m_thread;
};

//******************************************************************************
// Child-process exit codes (0 = the scenario's expectation was met)
//******************************************************************************

const int EXIT_MET                 = 0;
const int EXIT_WRONGLY_ACCEPTED    = 20;
const int EXIT_EXCEPTION_ESCAPED   = 21;
const int EXIT_WRONGLY_REJECTED    = 22;
const int EXIT_CONTENT_MISMATCH    = 23;
const int EXIT_SETUP_FAILED        = 24;

bool pairsEqual(const chaudiere::KeyValuePairs& kvp,
                const std::vector<std::pair<std::string, std::string>>& expected) {
   if (kvp.size() != expected.size()) {
      return false;
   }
   for (const auto& pair : expected) {
      if (!kvp.hasKey(pair.first) || (kvp.getValue(pair.first) != pair.second)) {
         return false;
      }
   }
   return true;
}

// Runs inside the forked child.
int runScenarioInChild(const ReceiveScenario& scenario) {
   LoopbackPair pair;
   if (!pair.create()) {
      printf("RESULT setup failed (loopback pair)\n");
      return EXIT_SETUP_FAILED;
   }

   chaudiere::Socket serverSocket(pair.releaseServerFd());
   RawPeer peer(pair.releasePeerFd(), scenario.segments, scenario.delayMillis, scenario.finish);
   peer.start();

   Message message;
   bool accepted = false;

   try {
      accepted = message.reconstitute(&serverSocket);
   } catch (const chaudiere::BasicException& be) {
      printf("RESULT exception escaped reconstitute(): BasicException '%s'\n", be.whatString().c_str());
      fflush(stdout);
      serverSocket.close();
      peer.releaseAndJoin();
      return EXIT_EXCEPTION_ESCAPED;
   } catch (const std::exception& e) {
      printf("RESULT exception escaped reconstitute(): std::exception '%s'\n", e.what());
      fflush(stdout);
      serverSocket.close();
      peer.releaseAndJoin();
      return EXIT_EXCEPTION_ESCAPED;
   } catch (...) {
      printf("RESULT exception escaped reconstitute(): unknown\n");
      fflush(stdout);
      serverSocket.close();
      peer.releaseAndJoin();
      return EXIT_EXCEPTION_ESCAPED;
   }

   // closing our end lets a peer that is still sending (or holding) finish
   serverSocket.close();
   peer.releaseAndJoin();

   const bool contentMatches =
      (message.getType() == scenario.expectedType) &&
      ((scenario.expectedType == MessageTypeText)
         ? (message.getTextPayload() == scenario.expectedText)
         : pairsEqual(message.getKeyValuesPayload(), scenario.expectedPairs));

   printf("RESULT reconstitute=%s type=%d textBytes=%zu pairs=%zu contentMatchesExpected=%s\n",
          accepted ? "true" : "false",
          static_cast<int>(message.getType()),
          message.getTextPayload().size(),
          message.getKeyValuesPayload().size(),
          contentMatches ? "yes" : "no");

   switch (scenario.expect) {
      case Expect::Reject:
         return accepted ? EXIT_WRONGLY_ACCEPTED : EXIT_MET;

      case Expect::Accept:
         if (!accepted) {
            return EXIT_WRONGLY_REJECTED;
         }
         return contentMatches ? EXIT_MET : EXIT_CONTENT_MISMATCH;

      case Expect::RejectOrAcceptExactly:
         if (!accepted) {
            return EXIT_MET;
         }
         return contentMatches ? EXIT_MET : EXIT_CONTENT_MISMATCH;
   }

   return EXIT_SETUP_FAILED;
}

std::string safeFileName(const std::string& id) {
   std::string name;
   for (char c : id) {
      name += (isalnum(static_cast<unsigned char>(c)) || (c == '.') || (c == '-')) ? c : '_';
   }
   return name;
}

std::string firstResultLine(const std::string& log) {
   const std::size_t pos = log.find("RESULT ");
   if (pos == std::string::npos) {
      return std::string();
   }
   std::size_t end = log.find('\n', pos);
   if (end == std::string::npos) {
      end = log.size();
   }
   return log.substr(pos + 7, end - pos - 7);
}

}

//******************************************************************************

TestReceivePath::TestReceivePath() :
   tonnerre_test::TonnerreTestSuite("TestReceivePath") {
}

//******************************************************************************

void TestReceivePath::runScenario(const ReceiveScenario& scenarioIn) {
   ReceiveScenario scenario = scenarioIn;
   scenario.defect = knownDefectFor(scenario.id);

   const Group group = runConfig().group;
   const bool isKnownDefect = !scenario.defect.empty();
   if (((group == Group::Unit) && isKnownDefect) ||
       ((group == Group::KnownDefects) && !isKnownDefect)) {
      return;
   }

   ScenarioTally& tally = scenarioTally();
   ++tally.run;

   const IsolatedResult result =
      runIsolated([&scenario]() { return runScenarioInChild(scenario); },
                  scenario.timeoutMillis);

   std::string problem;

   if (result.outcome == IsolatedResult::StartFailed) {
      problem = "could not start the isolated child process";
   } else if (result.outcome == IsolatedResult::TimedOut) {
      ++tally.timeouts;
      problem = "TIMEOUT: still blocked after " + std::to_string(scenario.timeoutMillis) +
                " ms, child killed";
   } else if (result.outcome == IsolatedResult::Signaled) {
      ++tally.crashes;
      problem = "CRASH: child terminated by " + signalName(result.signalNumber);
   } else if (!result.sanitizerFinding.empty()) {
      ++tally.sanitizerFindings;
      problem = "SANITIZER: " + result.sanitizerFinding;
   } else {
      switch (result.exitCode) {
         case EXIT_MET:
            break;
         case EXIT_WRONGLY_ACCEPTED:
            problem = "reconstitute() returned true for a message that must be rejected";
            break;
         case EXIT_EXCEPTION_ESCAPED:
            ++tally.escapedExceptions;
            problem = "an exception escaped reconstitute()";
            break;
         case EXIT_WRONGLY_REJECTED:
            problem = "reconstitute() returned false for a well-formed message";
            break;
         case EXIT_CONTENT_MISMATCH:
            problem = "reconstitute() returned true but the message differs from what was sent "
                      "(payload altered or silently discarded)";
            break;
         default:
            problem = "child exited with unexpected status " + std::to_string(result.exitCode);
            break;
      }
   }

   const bool passed = problem.empty();
   const std::string observed = firstResultLine(result.log);

   if (passed) {
      ++tally.passed;
      if (isKnownDefect) {
         ++tally.knownDefectNowPassing;
         printf("[XPASS %s] %s  -- annotated as a known defect but passes in this build (the defect may be observable only under a sanitizer, or it has been fixed)\n",
                scenario.defect.c_str(), scenario.id.c_str());
      } else {
         printf("[PASS] %s\n", scenario.id.c_str());
      }
   } else {
      if (isKnownDefect) {
         ++tally.knownDefectFailures;
         printf("[FAIL known-defect %s] %s\n", scenario.defect.c_str(), scenario.id.c_str());
      } else {
         ++tally.unexpectedFailures;
         printf("[FAIL unexpected] %s\n", scenario.id.c_str());
      }
      printf("       %s\n", scenario.description.c_str());
      printf("       %s\n", problem.c_str());
      if (!observed.empty()) {
         printf("       observed: %s\n", observed.c_str());
      }
   }
   fflush(stdout);

   // optionally keep every child's log (sanitizer reports, stack traces)
   const char* keepDir = std::getenv("TONNERRE_TEST_KEEP_LOGS");
   if ((keepDir != nullptr) && (*keepDir != '\0') && (!passed || !result.sanitizerFinding.empty())) {
      std::ofstream out(std::string(keepDir) + "/" + safeFileName(scenario.id) + ".log",
                        std::ios::binary);
      out << "scenario: " << scenario.id << "\n"
          << "description: " << scenario.description << "\n"
          << "problem: " << problem << "\n"
          << "elapsed_ms: " << result.elapsedMillis << "\n"
          << "----- child output -----\n"
          << result.log;
   }

   require(passed, scenario.id + ": " + (passed ? std::string("ok") : problem));
}

//******************************************************************************

void TestReceivePath::runTests() {
   testWellFormedMessages();
   testMalformedHeaderLengthPrefix();
   testMalformedPayloadLength();
   testMalformedHeaders();
   testTruncatedAtEveryByteOffset();
   testPeerResetDuringMessage();
   testPayloadSizeBoundaries();
   testStalledPeer();
}

//******************************************************************************
// Well-formed messages (the behavior that must keep working)
//******************************************************************************

void TestReceivePath::testWellFormedMessages() {
   TEST_CASE("receive path: well-formed messages are accepted intact");

   {
      ReceiveScenario s;
      s.id = "valid.text.small";
      s.description = "text message with an 11-byte payload";
      s.segments = { validTextWire("hello world") };
      s.expect = Expect::Accept;
      s.expectedText = "hello world";
      runScenario(s);
   }

   {
      ReceiveScenario s;
      s.id = "valid.text.empty-payload";
      s.description = "text message with an empty payload";
      s.segments = { validTextWire("") };
      s.expect = Expect::Accept;
      s.expectedText = "";
      runScenario(s);
   }

   {
      chaudiere::KeyValuePairs kvp;
      kvp.addPair("a", "1");
      kvp.addPair("b", "two");

      ReceiveScenario s;
      s.id = "valid.kvp.two-pairs";
      s.description = "key/values message with two simple pairs";
      s.segments = { validKvpWire(kvp) };
      s.expect = Expect::Accept;
      s.expectedType = MessageTypeKeyValues;
      s.expectedPairs = { {"a", "1"}, {"b", "two"} };
      runScenario(s);
   }

   {
      const std::string wire = validTextWire("fragmented delivery");
      ReceiveScenario s;
      s.id = "valid.text.one-byte-segments";
      s.description = "valid message delivered one byte per send() with 1 ms pauses";
      for (char c : wire) {
         s.segments.push_back(std::string(1, c));
      }
      s.delayMillis = 1;
      s.expect = Expect::Accept;
      s.expectedText = "fragmented delivery";
      runScenario(s);
   }

   {
      const std::string wire = validTextWire("split at the prefix boundary");
      ReceiveScenario s;
      s.id = "valid.text.split-after-prefix";
      s.description = "valid message delivered as 10-byte prefix, then the rest, 20 ms apart";
      s.segments = { wire.substr(0, 10), wire.substr(10) };
      s.delayMillis = 20;
      s.expect = Expect::Accept;
      s.expectedText = "split at the prefix boundary";
      runScenario(s);
   }

   // sizes around the 4096-byte stack/heap switch in readSocketBytes, and the
   // largest payload the current code supports
   const std::size_t sizes[] = { 1, 100, 4095, 4096, 4097, 32767 };
   for (std::size_t size : sizes) {
      const std::string text = repeatedChar('p', size);
      ReceiveScenario s;
      s.id = "valid.text.payload-" + std::to_string(size);
      s.description = "text message with a " + std::to_string(size) + "-byte payload";
      s.segments = { validTextWire(text) };
      s.expect = Expect::Accept;
      s.expectedText = text;
      runScenario(s);
   }
}

//******************************************************************************
// D-1 / D-4: malformed 10-character header-length prefix
//******************************************************************************

void TestReceivePath::testMalformedHeaderLengthPrefix() {
   TEST_CASE("receive path: malformed header-length prefixes are rejected safely");

   struct Prefix {
      const char* name;
      std::string bytes;     // sent as-is (already 10+ characters where relevant)
   };

   const std::vector<Prefix> prefixes = {
      { "negative-1",            pad10("-1") },
      { "negative-5",            pad10("-5") },
      { "int-min",               pad10("-2147483648") },
      { "negative-10-digits",    pad10("-999999999") },
      { "int-max",               pad10("2147483647") },
      { "int-max-plus-1",        pad10("2147483648") },
      { "int-max-plus-2",        pad10("2147483649") },
      { "uint-max",              pad10("4294967295") },
      { "uint-max-plus-1",       pad10("4294967296") },
      { "ten-nines",             pad10("9999999999") },
      { "twenty-nines",          std::string(20, '9') },
      { "ten-thousand-nines",    std::string(10000, '9') },
      { "just-over-32k",         pad10("32768") },
      { "max-segment-no-data",   pad10("32767") },
      { "empty-all-spaces",      std::string(10, ' ') },
      { "zero",                  pad10("0") },
      { "zeros",                 std::string(10, '0') },
      { "nonnumeric",            pad10("abcdefghij") },
      { "digits-then-letters",   pad10("12ab") },
      { "plus-sign",             pad10("+5") },
      { "decimal-point",         pad10("3.5") },
      { "hex",                   pad10("0x10") },
      { "exponent",              pad10("1e3") },
      { "leading-space",         " 12       " },
      { "embedded-nuls",         std::string(10, '\0') },
      { "nul-after-digits",      std::string("12\0\0\0\0\0\0\0\0", 10) },
   };

   for (const Prefix& prefix : prefixes) {
      ReceiveScenario s;
      s.id = std::string("header-length.") + prefix.name;
      s.description = "peer sends only a header-length prefix '" +
                      (prefix.bytes.size() <= 24 ? prefix.bytes : prefix.bytes.substr(0, 24) + "...") +
                      "' (" + std::to_string(prefix.bytes.size()) + " bytes) and closes";
      s.segments = { prefix.bytes };
      s.expect = Expect::Reject;
      runScenario(s);
   }

   // 4294967396 = 2^32 + 100: narrowed to int it becomes 100, so a receiver
   // that truncates the length will treat this as a 100-byte header
   {
      std::string header = "payload_type=text;payload_length=5;request=";
      header += std::string(100 - header.size(), 'x');
      ReceiveScenario s;
      s.id = "header-length.wraps-to-100-with-valid-100-byte-header";
      s.description = "prefix 4294967396 (2^32+100) followed by a valid 100-byte header and a 5-byte payload";
      s.segments = { pad10("4294967396") + header + "hello" };
      s.expect = Expect::Reject;
      runScenario(s);
   }

   {
      std::string header = "payload_type=text;payload_length=5;request=";
      header += std::string(50 - header.size(), 'x');
      ReceiveScenario s;
      s.id = "header-length.wraps-to-50-with-valid-50-byte-header";
      s.description = "prefix 4294967346 (2^32+50) followed by a valid 50-byte header and a 5-byte payload";
      s.segments = { pad10("4294967346") + header + "hello" };
      s.expect = Expect::Reject;
      runScenario(s);
   }

   // a hostile length followed by plenty of data, so the receiver cannot
   // "fail" merely because the peer closed early
   for (const char* length : { "4294967295", "2147483648", "-1" }) {
      ReceiveScenario s;
      s.id = std::string("header-length.with-trailing-data.") + length;
      s.description = std::string("prefix ") + length + " followed by 8 KiB of plausible data";
      s.segments = { pad10(length) + repeatedChar('x', 8192) };
      s.expect = Expect::Reject;
      runScenario(s);
   }
}

//******************************************************************************
// D-4: malformed payload_length header value
//******************************************************************************

void TestReceivePath::testMalformedPayloadLength() {
   TEST_CASE("receive path: malformed payload_length values are rejected safely");

   const std::vector<std::pair<std::string, std::string>> values = {
      { "negative-1",        "-1" },
      { "int-max",           "2147483647" },
      { "int-max-plus-1",    "2147483648" },
      { "uint-max",          "4294967295" },
      { "nineteen-nines",    "9999999999999999999" },
      { "twenty-nines",      "99999999999999999999" },
      { "nonnumeric",        "abc" },
      { "decimal",           "1.5" },
      { "exponent",          "1e3" },
      { "leading-space",     " 5" },
   };

   for (const auto& value : values) {
      ReceiveScenario s;
      s.id = "payload-length." + value.first;
      s.description = "header declares payload_length=" + value.second +
                      ", peer sends 5 payload bytes and closes";
      s.segments = { frame(textHeader(value.second), "hello") };
      s.expect = Expect::Reject;
      runScenario(s);
   }

   // 4294967301 = 2^32 + 5: narrowed to int it becomes 5, exactly the number
   // of payload bytes that follow
   {
      ReceiveScenario s;
      s.id = "payload-length.wraps-to-5-with-5-byte-payload";
      s.description = "payload_length=4294967301 (2^32+5), peer sends exactly 5 payload bytes";
      s.segments = { frame(textHeader("4294967301"), "hello") };
      s.expect = Expect::Reject;
      runScenario(s);
   }
}

//******************************************************************************
// Malformed headers
//******************************************************************************

void TestReceivePath::testMalformedHeaders() {
   TEST_CASE("receive path: malformed headers are rejected safely");

   const std::vector<std::pair<std::string, std::string>> headers = {
      { "missing-payload-type",   "payload_length=5;request=myRequest" },
      { "unknown-payload-type",   "payload_type=bogus;payload_length=5;request=myRequest" },
      { "no-equals-signs",        "this is not a header" },
      { "only-separators",        ";;;;;;;;" },
      { "binary-garbage",         std::string("\x01\x02\xff\xfe\x80;\x00=\x00", 10) },
   };

   for (const auto& header : headers) {
      ReceiveScenario s;
      s.id = "header." + header.first;
      s.description = "well-framed header that is not a valid message header";
      s.segments = { frame(header.second, "hello") };
      s.expect = Expect::Reject;
      runScenario(s);
   }
}

//******************************************************************************
// D-2: premature disconnect at every byte offset of a valid message
//******************************************************************************

void TestReceivePath::testTruncatedAtEveryByteOffset() {
   TEST_CASE("receive path: a message truncated at any byte offset is rejected");

   // a representative text message, then a key/values one; offset 0 is a peer
   // that connects and disconnects without sending anything; the full length
   // (a complete message) is covered by the well-formed tests
   const std::string textWire = validTextWire("hello world");

   chaudiere::KeyValuePairs kvp;
   kvp.addPair("a", "1");
   kvp.addPair("b", "two");
   const std::string kvpWire = validKvpWire(kvp);

   const std::pair<const char*, std::string> messages[] = {
      { "text", textWire },
      { "kvp",  kvpWire }
   };

   for (const auto& message : messages) {
      const std::string& wire = message.second;
      const std::string regionLabel = std::string(message.first);

      // locate the region boundaries for readable ids
      const std::size_t prefixEnd = 10;
      const std::size_t headerEnd = prefixEnd + static_cast<std::size_t>(
         std::atol(wire.substr(0, 10).c_str()));

      for (std::size_t offset = 0; offset < wire.size(); ++offset) {
         const char* region = (offset < prefixEnd) ? "prefix"
                              : (offset < headerEnd) ? "header" : "payload";

         ReceiveScenario s;
         s.id = "truncate." + regionLabel + "." + region + "@" +
                std::to_string(offset) + "of" + std::to_string(wire.size());
         s.description = "valid " + regionLabel + " message of " + std::to_string(wire.size()) +
                         " bytes; peer sends the first " + std::to_string(offset) +
                         " bytes (" + region + " region) and disconnects";
         s.segments = { wire.substr(0, offset) };
         s.expect = Expect::Reject;
         s.expectedType = (regionLabel == "kvp") ? MessageTypeKeyValues : MessageTypeText;
         runScenario(s);
      }
   }
}

//******************************************************************************

void TestReceivePath::testPeerResetDuringMessage() {
   TEST_CASE("receive path: a connection reset during a message is rejected");

   const std::string wire = validTextWire("hello world");
   const std::size_t headerEnd = 10 + static_cast<std::size_t>(std::atol(wire.substr(0, 10).c_str()));

   const std::pair<const char*, std::size_t> cutPoints[] = {
      { "no-bytes",       0 },
      { "inside-prefix",  4 },
      { "inside-header",  10 + 5 },
      { "end-of-header",  headerEnd },
      { "inside-payload", wire.size() - 3 }
   };

   for (const auto& cut : cutPoints) {
      ReceiveScenario s;
      s.id = std::string("reset.") + cut.first;
      s.description = "peer sends " + std::to_string(cut.second) + " of " +
                      std::to_string(wire.size()) + " bytes, then aborts the connection (RST)";
      s.segments = { wire.substr(0, cut.second) };
      s.finish = PeerFinish::Reset;
      s.expect = Expect::Reject;
      runScenario(s);
   }
}

//******************************************************************************
// D-3: payload-size boundaries
//******************************************************************************

void TestReceivePath::testPayloadSizeBoundaries() {
   TEST_CASE("receive path: payload sizes at and beyond the 32,767-byte limit are never silently discarded");

   // 32,767 is the largest payload the current code reads (valid.text.payload-32767
   // in the well-formed tests). Above it the supported maximum is a policy
   // decision for T1b: the only wrong answer is "accepted, but not what was sent".
   const std::size_t sizes[] = { 32768, 40000, 100000 };
   for (std::size_t size : sizes) {
      const std::string text = repeatedChar('q', size);
      ReceiveScenario s;
      s.id = "payload-size.text-" + std::to_string(size);
      s.description = "text message with a " + std::to_string(size) +
                      "-byte payload: reject it, or deliver all of it";
      s.segments = { validTextWire(text) };
      s.expect = Expect::RejectOrAcceptExactly;
      s.expectedText = text;
      runScenario(s);
   }

   // declared length beyond any plausible maximum, with only a few bytes sent
   const std::pair<const char*, const char*> declared[] = {
      { "int-max",        "2147483647" },
      { "one-hundred-million", "100000000" },
      { "forty-thousand", "40000" },
   };
   for (const auto& value : declared) {
      ReceiveScenario s;
      s.id = std::string("payload-size.declared-") + value.first + "-sent-10";
      s.description = std::string("header declares payload_length=") + value.second +
                      " but the peer sends only 10 payload bytes and disconnects";
      s.segments = { frame(textHeader(value.second), "0123456789") };
      s.expect = Expect::Reject;
      runScenario(s);
   }

   // key/values payload above the limit
   {
      chaudiere::KeyValuePairs kvp;
      std::vector<std::pair<std::string, std::string>> pairs;
      for (int i = 0; i < 3000; ++i) {
         const std::string key = "key" + std::to_string(i);
         const std::string value = "value" + std::to_string(i);
         kvp.addPair(key, value);
         pairs.push_back({key, value});
      }
      ReceiveScenario s;
      s.id = "payload-size.kvp-3000-pairs";
      s.description = "key/values message with 3000 simple pairs (well over 32 KiB of payload): "
                      "reject it, or deliver every pair";
      s.segments = { validKvpWire(kvp) };
      s.expect = Expect::RejectOrAcceptExactly;
      s.expectedType = MessageTypeKeyValues;
      s.expectedPairs = pairs;
      runScenario(s);
   }
}

//******************************************************************************
// D-8: a peer that connects, sends part of a message and then stays silent
//******************************************************************************

void TestReceivePath::testStalledPeer() {
   TEST_CASE("receive path: a stalled peer must not block the receiver indefinitely");

   const std::string wire = validTextWire("hello world");

   {
      ReceiveScenario s;
      s.id = "stalled.silent-after-connect";
      s.description = "peer connects, sends nothing and stays connected";
      s.segments = {};
      s.finish = PeerFinish::Hold;
      s.expect = Expect::Reject;
      s.timeoutMillis = 3000;
      runScenario(s);
   }

   {
      ReceiveScenario s;
      s.id = "stalled.silent-inside-prefix";
      s.description = "peer sends 4 of the 10 prefix bytes and stays connected";
      s.segments = { wire.substr(0, 4) };
      s.finish = PeerFinish::Hold;
      s.expect = Expect::Reject;
      s.timeoutMillis = 3000;
      runScenario(s);
   }

   {
      ReceiveScenario s;
      s.id = "stalled.silent-before-payload";
      s.description = "peer sends the prefix and header but no payload and stays connected";
      s.segments = { wire.substr(0, wire.size() - 11) };
      s.finish = PeerFinish::Hold;
      s.expect = Expect::Reject;
      s.timeoutMillis = 3000;
      runScenario(s);
   }
}
