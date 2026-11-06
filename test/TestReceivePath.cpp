// Copyright Paul Dardeau, SwampBits LLC 2026
// BSD License

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "TestReceivePath.h"
#include "IsolatedCase.h"
#include "LoopbackPair.h"
#include "Message.h"
#include "MessageHandler.h"
#include "MessageRequestHandler.h"
#include "Messaging.h"
#include "KeyValuePairs.h"
#include "BasicException.h"
#include "ServiceInfo.h"
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
   // a payload that was silently altered or discarded".
   RejectOrAcceptExactly
};

// what the scenario exercises
enum class Kind {
   Receive,                    // Message::reconstitute() against a scripted peer
   SendOversizeOneWay,         // Message::send(service) with an oversized message
   SendOversizeWithResponse,   // Message::send(service, response), oversized
   SendBadResponseNotPooled,   // a failed response must not leave a pooled socket
   SendGoodResponsePooled,     // control: a good response still pools the socket
   ServerResponse              // MessageRequestHandler writing a response of a given size
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
   Kind kind = Kind::Receive;
   MessageType initialType = MessageTypeUnknown;  // state of the Message before reconstitute()
   std::string initialText;
   std::size_t payloadSize = 0;          // send/serve kinds: payload bytes to produce
};

}

//******************************************************************************
// Known defects (T0 report, section 9). A scenario listed here fails today and
// is reported as a known-defect scenario: it is expected to fail until the
// defect is fixed, and it still counts as a failure in the run. The list is
// never used to skip a scenario or to relax an expectation.
//
// T1a listed 103 scenarios (defects D-1, D-2, D-3, D-4, D-8 and D-15). T1b fixed
// the receive path (Message.cpp reconstitute()) and removed the 100 entries for
// D-1, D-2, D-3, D-4 and D-15 after verifying that each passes, in Debug and in
// ASan+UBSan builds, and across repeated runs. What remains:
//
//   D-8   no receive timeout: a stalled peer blocks the receiver forever
//         (T1c). The scenarios can only assert that the receiver gives up; the
//         timeout policy does not exist yet.
//******************************************************************************

namespace {

struct DefectNote {
   const char* id;
   const char* defect;
};

const DefectNote KNOWN_DEFECTS[] = {
   { "stalled.silent-after-connect", "D-8" },
   { "stalled.silent-before-payload", "D-8" },
   { "stalled.silent-inside-prefix", "D-8" },
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
const int EXIT_STATE_NOT_CLEARED   = 25;   // a rejected message still holds received state
const int EXIT_CONNECTION_OPEN     = 26;   // the connection was not closed after a rejection
const int EXIT_STREAM_REUSABLE     = 27;   // a second reconstitute() on the same socket succeeded
const int EXIT_OVERSIZE_SENT       = 30;   // an oversized message reached the wire
const int EXIT_DEAD_SOCKET_POOLED  = 31;   // a closed connection was returned to the pool
const int EXIT_POOL_BROKEN         = 32;   // a healthy connection was not pooled
const int EXIT_SEND_WRONG_RESULT   = 33;   // send() returned the wrong result
const int EXIT_RESPONSE_MISMATCH   = 34;   // the served response is not what the limits require

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

int runReceiveScenarioInChild(const ReceiveScenario& scenario);
int runSendScenarioInChild(const ReceiveScenario& scenario);
int runServeScenarioInChild(const ReceiveScenario& scenario);

// Runs inside the forked child.
int runScenarioInChild(const ReceiveScenario& scenario) {
   switch (scenario.kind) {
      case Kind::Receive:
         return runReceiveScenarioInChild(scenario);
      case Kind::ServerResponse:
         return runServeScenarioInChild(scenario);
      default:
         return runSendScenarioInChild(scenario);
   }
}

int runReceiveScenarioInChild(const ReceiveScenario& scenario) {
   LoopbackPair pair;
   if (!pair.create()) {
      printf("RESULT setup failed (loopback pair)\n");
      return EXIT_SETUP_FAILED;
   }

   chaudiere::Socket serverSocket(pair.releaseServerFd());
   RawPeer peer(pair.releasePeerFd(), scenario.segments, scenario.delayMillis, scenario.finish);
   peer.start();

   Message message;
   message.setType(scenario.initialType);
   message.setTextPayload(scenario.initialText);
   bool accepted = false;
   int afterRejection = EXIT_MET;

   try {
      accepted = message.reconstitute(&serverSocket);

      if (!accepted) {
         // after a rejection: nothing received is left in the message, the
         // connection is closed, and whatever bytes remain unread cannot be
         // read as another message
         if ((message.getType() != MessageTypeUnknown) ||
             !message.getTextPayload().empty() ||
             (message.getKeyValuesPayload().size() != 0) ||
             message.hasHeader("payload_type") ||
             message.hasHeader("payload_length")) {
            afterRejection = EXIT_STATE_NOT_CLEARED;
         } else if (serverSocket.isConnected()) {
            afterRejection = EXIT_CONNECTION_OPEN;
         } else {
            Message second;
            if (second.reconstitute(&serverSocket)) {
               afterRejection = EXIT_STREAM_REUSABLE;
            }
         }
      }
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
         return accepted ? EXIT_WRONGLY_ACCEPTED : afterRejection;

      case Expect::Accept:
         if (!accepted) {
            return EXIT_WRONGLY_REJECTED;
         }
         return contentMatches ? EXIT_MET : EXIT_CONTENT_MISMATCH;

      case Expect::RejectOrAcceptExactly:
         if (!accepted) {
            return afterRejection;
         }
         return contentMatches ? EXIT_MET : EXIT_CONTENT_MISMATCH;
   }

   return EXIT_SETUP_FAILED;
}

//******************************************************************************
// Send path and server path (T1b): size limits on outgoing messages and what
// happens to the connection after a failed exchange
//******************************************************************************

// a listening socket on 127.0.0.1 with a kernel-chosen port
class ListeningEndpoint {
public:
   ListeningEndpoint() : m_fd(-1), m_port(0) {
      m_fd = ::socket(AF_INET, SOCK_STREAM, 0);
      if (m_fd < 0) {
         return;
      }
      LoopbackPair::allowAddressReuse(m_fd);

      sockaddr_in addr;
      std::memset(&addr, 0, sizeof(addr));
      addr.sin_family = AF_INET;
      addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
      addr.sin_port = 0;

      socklen_t addrLen = sizeof(addr);
      if ((::bind(m_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) ||
          (::listen(m_fd, 4) != 0) ||
          (::getsockname(m_fd, reinterpret_cast<sockaddr*>(&addr), &addrLen) != 0)) {
         ::close(m_fd);
         m_fd = -1;
         return;
      }
      m_port = ntohs(addr.sin_port);
   }

   ~ListeningEndpoint() {
      if (m_fd >= 0) {
         ::close(m_fd);
      }
   }

   bool ok() const { return m_fd >= 0; }
   int fd() const { return m_fd; }
   unsigned short port() const { return m_port; }

   // true if a connection is waiting to be accepted within the given time
   bool connectionPending(int waitMillis) const {
      pollfd p;
      p.fd = m_fd;
      p.events = POLLIN;
      p.revents = 0;
      return ::poll(&p, 1, waitMillis) > 0;
   }

private:
   int m_fd;
   unsigned short m_port;
};

// bytes that arrive on fd within waitMillis (or until the peer closes)
std::size_t bytesArriving(int fd, int waitMillis) {
   std::size_t total = 0;
   char buffer[4096];

   while (true) {
      pollfd p;
      p.fd = fd;
      p.events = POLLIN;
      p.revents = 0;
      if (::poll(&p, 1, waitMillis) <= 0) {
         break;
      }
      const ssize_t n = ::recv(fd, buffer, sizeof(buffer), 0);
      if (n <= 0) {
         break;
      }
      total += static_cast<std::size_t>(n);
   }

   return total;
}

// accepts one connection, reads the request, writes 'reply', and then either
// holds the connection open or closes it
class ServicePeer {
public:
   ServicePeer(const ListeningEndpoint& endpoint, const std::string& reply, bool hold) :
      m_endpoint(endpoint), m_reply(reply), m_hold(hold), m_release(false) {
   }

   void start() {
      m_thread = std::thread([this]() { run(); });
   }

   void releaseAndJoin() {
      m_release = true;
      if (m_thread.joinable()) {
         m_thread.join();
      }
   }

private:
   void run() {
      if (!m_endpoint.connectionPending(3000)) {
         return;
      }
      const int fd = ::accept(m_endpoint.fd(), nullptr, nullptr);
      if (fd < 0) {
         return;
      }

      // read the request: everything that arrives until a pause
      bytesArriving(fd, 300);

      std::size_t sent = 0;
      while (sent < m_reply.size()) {
         const ssize_t rc = ::send(fd, m_reply.data() + sent, m_reply.size() - sent, MSG_NOSIGNAL);
         if (rc <= 0) {
            break;
         }
         sent += static_cast<std::size_t>(rc);
      }

      if (m_hold) {
         while (!m_release) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
         }
      } else {
         ::shutdown(fd, SHUT_WR);
      }
      ::close(fd);
   }

   const ListeningEndpoint& m_endpoint;
   std::string m_reply;
   bool m_hold;
   std::atomic<bool> m_release;
   std::thread m_thread;
};

int runSendScenarioInChild(const ReceiveScenario& scenario) {
   ListeningEndpoint endpoint;
   if (!endpoint.ok()) {
      printf("RESULT setup failed (listening socket)\n");
      return EXIT_SETUP_FAILED;
   }

   Messaging* messagingObject = new Messaging();
   chaudiere::ServiceInfo info("t1bService", "127.0.0.1", endpoint.port());
   info.setPersistentConnection(true);
   messagingObject->registerService("t1bService", info);
   Messaging::setMessaging(messagingObject);
   std::shared_ptr<Messaging> messaging = Messaging::getMessaging();

   Message request("t1bRequest", MessageTypeText);
   request.setTextPayload(std::string(scenario.payloadSize, 'p'));
   Message response;

   if ((scenario.kind == Kind::SendOversizeOneWay) ||
       (scenario.kind == Kind::SendOversizeWithResponse)) {
      const bool sent = (scenario.kind == Kind::SendOversizeOneWay)
                        ? request.send("t1bService")
                        : request.send("t1bService", response);
      printf("RESULT send=%s\n", sent ? "true" : "false");
      if (sent) {
         return EXIT_SEND_WRONG_RESULT;
      }

      // nothing may have reached the wire: no connection, or a connection
      // that carried no bytes
      if (endpoint.connectionPending(200)) {
         const int fd = ::accept(endpoint.fd(), nullptr, nullptr);
         const std::size_t arrived = (fd >= 0) ? bytesArriving(fd, 200) : 0;
         if (fd >= 0) {
            ::close(fd);
         }
         if (arrived > 0) {
            printf("RESULT %zu bytes of an oversized message reached the wire\n", arrived);
            return EXIT_OVERSIZE_SENT;
         }
      }
      return EXIT_MET;
   }

   const bool good = (scenario.kind == Kind::SendGoodResponsePooled);
   std::string reply;
   if (good) {
      reply = validTextWire("ok");
   } else {
      // a response that stops in the middle of its payload
      const std::string whole = validTextWire("a response that never completes");
      reply = whole.substr(0, whole.size() - 8);
   }

   ServicePeer peer(endpoint, reply, good);
   peer.start();

   const bool received = request.send("t1bService", response);
   printf("RESULT send=%s\n", received ? "true" : "false");

   int result = EXIT_MET;
   if (received != good) {
      result = EXIT_SEND_WRONG_RESULT;
   } else {
      // whatever the pool now hands out must be a usable connection when the
      // exchange succeeded, and must not be the failed one when it did not
      chaudiere::Socket* next = messaging->socketForService(info);
      if (next == nullptr) {
         result = EXIT_POOL_BROKEN;
      } else if (!next->isConnected()) {
         result = good ? EXIT_POOL_BROKEN : EXIT_DEAD_SOCKET_POOLED;
      } else if (good) {
         // pooled and reused: no second connection was opened
         if (endpoint.connectionPending(150)) {
            result = EXIT_POOL_BROKEN;
         }
      }
      delete next;
   }

   peer.releaseAndJoin();
   return result;
}

// answers every text request with a payload of a configured size
class SizedResponseHandler : public tonnerre::MessageHandler {
public:
   explicit SizedResponseHandler(std::size_t size) : m_size(size) {
   }

   void handleTextMessage(const Message&,
                          Message&,
                          const std::string&,
                          const std::string&,
                          std::string& responsePayload) override {
      responsePayload.assign(m_size, 'r');
   }

   void handleKeyValuesMessage(const Message&,
                               Message&,
                               const std::string&,
                               const chaudiere::KeyValuePairs&,
                               chaudiere::KeyValuePairs&) override {
   }

private:
   std::size_t m_size;
};

int runServeScenarioInChild(const ReceiveScenario& scenario) {
   LoopbackPair pair;
   if (!pair.create()) {
      printf("RESULT setup failed (loopback pair)\n");
      return EXIT_SETUP_FAILED;
   }

   const int clientFd = pair.releasePeerFd();
   // the request handler takes ownership of the socket and closes it when deleted
   chaudiere::Socket* serverSocket = new chaudiere::Socket(pair.releaseServerFd());

   const std::string requestWire = validTextWire("please respond", "sizedRequest");
   if (::send(clientFd, requestWire.data(), requestWire.size(), MSG_NOSIGNAL) !=
       static_cast<ssize_t>(requestWire.size())) {
      ::close(clientFd);
      return EXIT_SETUP_FAILED;
   }

   SizedResponseHandler handler(scenario.payloadSize);
   MessageRequestHandler* requestHandler = new MessageRequestHandler(serverSocket, &handler);
   requestHandler->run();
   delete requestHandler;

   // the response the server is allowed to send, if any
   Message expected("sizedRequest", MessageTypeText);
   expected.setTextPayload(std::string(scenario.payloadSize, 'r'));
   const std::size_t expectedBytes =
      (scenario.payloadSize <= 32767) ? expected.toString().size() : 0;

   const std::size_t arrived = bytesArriving(clientFd, 1000);
   ::close(clientFd);

   printf("RESULT served payload=%zu bytes arrived=%zu expected=%zu\n",
          scenario.payloadSize, arrived, expectedBytes);
   return (arrived == expectedBytes) ? EXIT_MET : EXIT_RESPONSE_MISMATCH;
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
         case EXIT_STATE_NOT_CLEARED:
            problem = "reconstitute() returned false but left received state in the message";
            break;
         case EXIT_CONNECTION_OPEN:
            problem = "reconstitute() returned false but left the connection open";
            break;
         case EXIT_STREAM_REUSABLE:
            problem = "after a rejection, a second reconstitute() on the same socket read more data";
            break;
         case EXIT_OVERSIZE_SENT:
            problem = "an oversized message was written to the connection";
            break;
         case EXIT_DEAD_SOCKET_POOLED:
            problem = "a connection that failed mid-exchange was returned to the connection pool";
            break;
         case EXIT_POOL_BROKEN:
            problem = "a healthy connection was not returned to the pool (or the pool misbehaved)";
            break;
         case EXIT_SEND_WRONG_RESULT:
            problem = "send() returned the wrong result";
            break;
         case EXIT_RESPONSE_MISMATCH:
            problem = "the server sent a different number of response bytes than the size limit allows";
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
      ReceiveScenario s;
      s.id = "valid.text.empty-request-name";
      s.description = "text message whose request name is empty (written as 'request=')";
      s.segments = { validTextWire("hello", "") };
      s.expect = Expect::Accept;
      s.expectedText = "hello";
      runScenario(s);
   }

   {
      const std::string text("ab\0cd\0", 6);
      ReceiveScenario s;
      s.id = "valid.text.embedded-nul-payload";
      s.description = "text payload containing NUL bytes is delivered whole (not cut at the first NUL)";
      s.segments = { validTextWire(text) };
      s.expect = Expect::Accept;
      s.expectedText = text;
      runScenario(s);
   }

   {
      ReceiveScenario s;
      s.id = "valid.text.one-way-header";
      s.description = "text message with a 1way=true header";
      s.segments = { frame("payload_type=text;payload_length=5;request=r;1way=true", "hello") };
      s.expect = Expect::Accept;
      s.expectedText = "hello";
      runScenario(s);
   }

   {
      const std::string wire = validTextWire("zero-padded prefix");
      const std::string length = std::to_string(std::atol(wire.substr(0, 10).c_str()));
      ReceiveScenario s;
      s.id = "valid.text.zero-padded-prefix";
      s.description = "valid message whose header-length prefix is padded with leading zeros";
      s.segments = { std::string(10 - length.size(), '0') + length + wire.substr(10) };
      s.expect = Expect::Accept;
      s.expectedText = "zero-padded prefix";
      runScenario(s);
   }

   {
      const std::string wire = validTextWire("two in a row") + validTextWire("second");
      ReceiveScenario s;
      s.id = "valid.text.followed-by-another-message";
      s.description = "two valid messages back to back: the first is delivered intact "
                      "(and does not consume the second)";
      s.segments = { wire };
      s.expect = Expect::Accept;
      s.expectedText = "two in a row";
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
      { "missing-payload-length", "payload_type=text;request=myRequest" },
      { "empty-payload-length",   "payload_type=text;payload_length=;request=myRequest" },
      { "empty-payload-type",     "payload_type=;payload_length=5;request=myRequest" },
      { "unknown-type-literal",   "payload_type=unknown;payload_length=5;request=myRequest" },
      { "duplicate-payload-length", "payload_type=text;payload_length=5;payload_length=5;request=myRequest" },
      { "conflicting-payload-length", "payload_type=text;payload_length=5;payload_length=6;request=myRequest" },
      { "duplicate-payload-type", "payload_type=text;payload_type=text;payload_length=5" },
      { "two-equals-in-pair",     "payload_type=text;payload_length=5;request=a=b" },
      { "empty-pair-middle",      "payload_type=text;;payload_length=5" },
      { "empty-pair-trailing",    "payload_type=text;payload_length=5;" },
      { "empty-pair-leading",     ";payload_type=text;payload_length=5" },
      { "empty-key",              "payload_type=text;payload_length=5;=x" },
      { "pair-without-equals",    "payload_type=text;payload_length=5;orphan" },
      { "type-case-mismatch",     "payload_type=TEXT;payload_length=5" },
      { "payload-length-with-sign", "payload_type=text;payload_length=+5" },
      { "payload-length-trailing-space", "payload_type=text;payload_length=5 " },
   };

   for (const auto& header : headers) {
      ReceiveScenario s;
      s.id = "header." + header.first;
      s.description = "well-framed header that is not a valid message header";
      s.segments = { frame(header.second, "hello") };
      s.expect = Expect::Reject;
      runScenario(s);
   }

   // a message that already holds a type and a payload must not be left
   // looking received (or keep its type) when the new message is rejected
   {
      ReceiveScenario s;
      s.id = "header.unknown-type-on-preset-message";
      s.description = "message already typed text with a stale payload receives payload_type=bogus";
      s.segments = { frame("payload_type=bogus;payload_length=5;request=myRequest", "hello") };
      s.initialType = MessageTypeText;
      s.initialText = "stale";
      s.expect = Expect::Reject;
      runScenario(s);
   }

   {
      ReceiveScenario s;
      s.id = "truncated.clears-preset-state";
      s.description = "message already typed text with a stale payload receives a truncated message";
      const std::string wire = validTextWire("hello world");
      s.segments = { wire.substr(0, wire.size() - 3) };
      s.initialType = MessageTypeText;
      s.initialText = "stale";
      s.expect = Expect::Reject;
      runScenario(s);
   }

   // and a message that is received replaces what it held
   {
      ReceiveScenario s;
      s.id = "valid.replaces-preset-payload";
      s.description = "message holding a stale payload receives a valid message with an empty payload";
      s.segments = { validTextWire("") };
      s.initialType = MessageTypeText;
      s.initialText = "stale";
      s.expect = Expect::Accept;
      s.expectedText = "";
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
   TEST_CASE("receive path: payloads above the 32,767-byte limit are rejected, never silently discarded");

   // 32,767 is the maximum payload (valid.text.payload-32767 in the well-formed
   // tests accepts it intact); anything larger is rejected, and the receiver
   // never reports success for a message whose payload it did not deliver
   const std::size_t sizes[] = { 32768, 40000, 100000 };
   for (std::size_t size : sizes) {
      const std::string text = repeatedChar('q', size);
      ReceiveScenario s;
      s.id = "payload-size.text-" + std::to_string(size);
      s.description = "text message with a " + std::to_string(size) +
                      "-byte payload (over the maximum) must be rejected";
      s.segments = { validTextWire(text) };
      s.expect = Expect::Reject;
      runScenario(s);
   }

   // the unread bytes of an oversized message must not be interpreted as a
   // message: the oversized message is followed by a complete, valid one
   {
      ReceiveScenario s;
      s.id = "payload-size.oversized-then-valid-message";
      s.description = "a 40,000-byte message followed by a valid message: the first is rejected "
                      "and the connection yields nothing further";
      s.segments = { validTextWire(repeatedChar('q', 40000)) + validTextWire("second") };
      s.expect = Expect::Reject;
      runScenario(s);
   }

   {
      ReceiveScenario s;
      s.id = "payload-size.declared-oversize-then-valid-message";
      s.description = "header declares payload_length=40000, then a complete valid message arrives "
                      "(the 'payload' would swallow it): rejected, and nothing further is read";
      s.segments = { frame(textHeader("40000"), "") + validTextWire("second") };
      s.expect = Expect::Reject;
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
      for (int i = 0; i < 3000; ++i) {
         kvp.addPair("key" + std::to_string(i), "value" + std::to_string(i));
      }
      ReceiveScenario s;
      s.id = "payload-size.kvp-3000-pairs";
      s.description = "key/values message with 3000 simple pairs (well over the 32,767-byte maximum "
                      "of payload) must be rejected";
      s.segments = { validKvpWire(kvp) };
      s.expect = Expect::Reject;
      s.expectedType = MessageTypeKeyValues;
      runScenario(s);
   }

   // header size limit: 32,767 is accepted, 32,768 is not
   {
      std::string header = "payload_type=text;payload_length=0;request=";
      header += std::string(32767 - header.size(), 'h');
      ReceiveScenario s;
      s.id = "header-size.at-limit-32767";
      s.description = "valid message whose header is exactly 32,767 bytes";
      s.segments = { frame(header, "") };
      s.expect = Expect::Accept;
      s.expectedText = "";
      runScenario(s);
   }

   {
      std::string header = "payload_type=text;payload_length=0;request=";
      header += std::string(32768 - header.size(), 'h');
      ReceiveScenario s;
      s.id = "header-size.over-limit-32768";
      s.description = "header of 32,768 bytes (over the maximum) must be rejected";
      s.segments = { frame(header, "") };
      s.expect = Expect::Reject;
      runScenario(s);
   }

   // sending: an oversized message is not sent; the server does not send an
   // oversized response
   const std::size_t oversize = 32768;
   {
      ReceiveScenario s;
      s.id = "send.oversized-one-way";
      s.description = "Message::send(service) with a 32,768-byte payload fails and writes nothing";
      s.kind = Kind::SendOversizeOneWay;
      s.payloadSize = oversize;
      runScenario(s);
   }

   {
      ReceiveScenario s;
      s.id = "send.oversized-with-response";
      s.description = "Message::send(service, response) with a 32,768-byte payload fails and writes nothing";
      s.kind = Kind::SendOversizeWithResponse;
      s.payloadSize = oversize;
      runScenario(s);
   }

   {
      ReceiveScenario s;
      s.id = "serve.response-over-limit";
      s.description = "a handler returns a 32,768-byte response: the server sends nothing";
      s.kind = Kind::ServerResponse;
      s.payloadSize = oversize;
      runScenario(s);
   }

   {
      ReceiveScenario s;
      s.id = "serve.response-at-limit";
      s.description = "a handler returns a 32,767-byte response: the server sends all of it";
      s.kind = Kind::ServerResponse;
      s.payloadSize = 32767;
      runScenario(s);
   }

   // connection state after a failed exchange
   {
      ReceiveScenario s;
      s.id = "pool.failed-response-is-not-pooled";
      s.description = "persistent service: the response stops mid-payload; the failed connection is "
                      "not returned to the pool";
      s.kind = Kind::SendBadResponseNotPooled;
      s.payloadSize = 5;
      runScenario(s);
   }

   {
      ReceiveScenario s;
      s.id = "pool.good-response-is-pooled";
      s.description = "persistent service: a good response still returns the connection to the pool";
      s.kind = Kind::SendGoodResponsePooled;
      s.payloadSize = 5;
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
