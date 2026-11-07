// Copyright Paul Dardeau, SwampBits LLC 2026
// BSD License

#include <atomic>
#include <chrono>
#include <climits>
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
#include <pthread.h>
#include <signal.h>
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
   ServerResponse,             // MessageRequestHandler writing a response of a given size
   ResponseDeadline,           // send(service, response, deadline) against a slow handler
   NegativeReconstituteDeadline,   // reconstitute(socket, negative) is rejected untouched
   NegativeSendDeadline        // send(service, response, negative) is rejected, nothing sent
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
   int timeoutMillis = 8000;             // hard limit for the child process (the harness backstop)
   Kind kind = Kind::Receive;
   MessageType initialType = MessageTypeUnknown;  // state of the Message before reconstitute()
   std::string initialText;
   std::size_t payloadSize = 0;          // send/serve kinds: payload bytes to produce

   // receive deadline (T1c). The scenarios use sub-second deadlines, passed
   // explicitly, so that the 10 s production default is never shortened for the
   // harness's sake.
   int deadlineMillis = -1;              // Receive: reconstitute(socket, n); -1: reconstitute(socket), the default
                                         // ResponseDeadline: the short response deadline
   int longDeadlineMillis = 0;           // ResponseDeadline: the longer one that must succeed
   int handlerDelayMillis = 0;           // ResponseDeadline: how long the server's handler takes
   int initialDelayMillis = 0;           // the peer waits this long before its first byte
   int settleMillis = 0;                 // the receiver waits this long before it starts reading
   int signalIntervalMillis = 0;         // > 0: SIGUSR1 to the receiving thread this often (no SA_RESTART)
   int minElapsedMillis = 0;             // Receive/ResponseDeadline: required elapsed time (0: not checked)
   int maxElapsedMillis = 0;
   bool slowOptIn = false;               // runs only if TONNERRE_TEST_SLOW is set (waits out the 10 s default)
};

}

//******************************************************************************
// History: T1a recorded 103 scenarios as known defects (T0 defects D-1, D-2, D-3,
// D-4, D-8 and D-15). T1b fixed the receive-path framing (100 of them) and T1c
// the receive deadline (the 3 D-8 stalled-peer scenarios). There is no longer a
// known-defect table: every scenario is an ordinary test, and a failure is a
// failure.
//******************************************************************************

namespace {

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
           PeerFinish finish,
           int initialDelayMillis = 0) :
      m_fd(fd),
      m_segments(segments),
      m_delayMillis(delayMillis),
      m_finish(finish),
      m_initialDelayMillis(initialDelayMillis),
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
      if (m_initialDelayMillis > 0) {
         std::this_thread::sleep_for(std::chrono::milliseconds(m_initialDelayMillis));
      }

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
   int m_initialDelayMillis;
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
const int EXIT_TOO_EARLY           = 35;   // gave up before the deadline could have passed
const int EXIT_TOO_LATE            = 36;   // took much longer than the deadline
const int EXIT_DEADLINE_OVERRIDE   = 37;   // a response deadline was not honoured
const int EXIT_NEGATIVE_MISHANDLED = 38;   // a negative deadline was not rejected cleanly
const int EXIT_NO_SIGNALS          = 39;   // the signal storm delivered no signal (the test would prove nothing)

//******************************************************************************
// Signals: SIGUSR1 delivered to the receiving thread every few milliseconds, with
// a handler installed without SA_RESTART, so that a poll() that is waiting is
// interrupted (EINTR) over and over

volatile sig_atomic_t g_stormSignals = 0;

void onStormSignal(int) {
   g_stormSignals = g_stormSignals + 1;
}

class SignalStorm {
public:
   explicit SignalStorm(int intervalMillis) :
      m_target(pthread_self()),
      m_intervalMillis(intervalMillis),
      m_stop(false),
      m_installed(false) {
      g_stormSignals = 0;

      struct sigaction action;
      std::memset(&action, 0, sizeof(action));
      action.sa_handler = onStormSignal;
      ::sigemptyset(&action.sa_mask);
      action.sa_flags = 0;   // deliberately no SA_RESTART
      m_installed = (::sigaction(SIGUSR1, &action, &m_previous) == 0);

      if (m_installed) {
         m_thread = std::thread([this]() { run(); });
      }
   }

   ~SignalStorm() {
      m_stop = true;
      if (m_thread.joinable()) {
         m_thread.join();
      }
      if (m_installed) {
         ::sigaction(SIGUSR1, &m_previous, nullptr);
      }
   }

   SignalStorm(const SignalStorm&) = delete;
   SignalStorm& operator=(const SignalStorm&) = delete;

   bool installed() const { return m_installed; }

private:
   void run() {
      while (!m_stop) {
         ::pthread_kill(m_target, SIGUSR1);
         std::this_thread::sleep_for(std::chrono::milliseconds(m_intervalMillis));
      }
   }

   pthread_t m_target;
   int m_intervalMillis;
   std::atomic<bool> m_stop;
   bool m_installed;
   struct sigaction m_previous;
   std::thread m_thread;
};

long long millisSince(std::chrono::steady_clock::time_point since) {
   return std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - since).count();
}

// elapsed time against the scenario's bounds; EXIT_MET if there are none or they hold
int timingResult(const ReceiveScenario& scenario, long long elapsedMillis) {
   if ((scenario.minElapsedMillis > 0) && (elapsedMillis < scenario.minElapsedMillis)) {
      return EXIT_TOO_EARLY;
   }
   if ((scenario.maxElapsedMillis > 0) && (elapsedMillis > scenario.maxElapsedMillis)) {
      return EXIT_TOO_LATE;
   }
   return EXIT_MET;
}

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
int runResponseDeadlineScenarioInChild(const ReceiveScenario& scenario);
int runNegativeReconstituteScenarioInChild(const ReceiveScenario& scenario);
int runNegativeSendScenarioInChild(const ReceiveScenario& scenario);

// Runs inside the forked child.
int runScenarioInChild(const ReceiveScenario& scenario) {
   switch (scenario.kind) {
      case Kind::Receive:
         return runReceiveScenarioInChild(scenario);
      case Kind::ServerResponse:
         return runServeScenarioInChild(scenario);
      case Kind::ResponseDeadline:
         return runResponseDeadlineScenarioInChild(scenario);
      case Kind::NegativeReconstituteDeadline:
         return runNegativeReconstituteScenarioInChild(scenario);
      case Kind::NegativeSendDeadline:
         return runNegativeSendScenarioInChild(scenario);
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
   RawPeer peer(pair.releasePeerFd(),
                scenario.segments,
                scenario.delayMillis,
                scenario.finish,
                scenario.initialDelayMillis);
   peer.start();

   Message message;
   message.setType(scenario.initialType);
   message.setTextPayload(scenario.initialText);
   bool accepted = false;
   int afterRejection = EXIT_MET;
   long long elapsedMillis = 0;
   int signalsDelivered = 0;

   // lets bytes that are already on their way arrive before reading starts
   if (scenario.settleMillis > 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(scenario.settleMillis));
   }

   // declared after the peer, so it is stopped first
   std::unique_ptr<SignalStorm> storm;
   if (scenario.signalIntervalMillis > 0) {
      storm.reset(new SignalStorm(scenario.signalIntervalMillis));
      if (!storm->installed()) {
         printf("RESULT setup failed (signal handler)\n");
         return EXIT_SETUP_FAILED;
      }
   }

   try {
      const std::chrono::steady_clock::time_point began = std::chrono::steady_clock::now();
      accepted = (scenario.deadlineMillis >= 0)
                 ? message.reconstitute(&serverSocket, scenario.deadlineMillis)
                 : message.reconstitute(&serverSocket);
      elapsedMillis = millisSince(began);

      if (storm) {
         storm.reset();
         signalsDelivered = g_stormSignals;
      }

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

   printf("RESULT reconstitute=%s type=%d textBytes=%zu pairs=%zu contentMatchesExpected=%s elapsedMs=%lld signals=%d\n",
          accepted ? "true" : "false",
          static_cast<int>(message.getType()),
          message.getTextPayload().size(),
          message.getKeyValuesPayload().size(),
          contentMatches ? "yes" : "no",
          elapsedMillis,
          signalsDelivered);

   int result = EXIT_SETUP_FAILED;
   switch (scenario.expect) {
      case Expect::Reject:
         result = accepted ? EXIT_WRONGLY_ACCEPTED : afterRejection;
         break;

      case Expect::Accept:
         if (!accepted) {
            result = EXIT_WRONGLY_REJECTED;
         } else {
            result = contentMatches ? EXIT_MET : EXIT_CONTENT_MISMATCH;
         }
         break;

      case Expect::RejectOrAcceptExactly:
         if (!accepted) {
            result = afterRejection;
         } else {
            result = contentMatches ? EXIT_MET : EXIT_CONTENT_MISMATCH;
         }
         break;
   }

   if (result == EXIT_MET) {
      result = timingResult(scenario, elapsedMillis);
   }

   // a signal test in which no signal arrived would prove nothing
   if ((result == EXIT_MET) && (scenario.signalIntervalMillis > 0) && (signalsDelivered == 0)) {
      result = EXIT_NO_SIGNALS;
   }

   return result;
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

//******************************************************************************
// Receive deadline for the response (T1c): send(service, response, deadline)
//******************************************************************************

// answers every text request, after a delay
class SlowResponseHandler : public tonnerre::MessageHandler {
public:
   explicit SlowResponseHandler(int delayMillis) : m_delayMillis(delayMillis) {
   }

   void handleTextMessage(const Message&,
                          Message&,
                          const std::string&,
                          const std::string&,
                          std::string& responsePayload) override {
      std::this_thread::sleep_for(std::chrono::milliseconds(m_delayMillis));
      responsePayload = "slow reply";
   }

   void handleKeyValuesMessage(const Message&,
                               Message&,
                               const std::string&,
                               const chaudiere::KeyValuePairs&,
                               chaudiere::KeyValuePairs&) override {
   }

private:
   int m_delayMillis;
};

// Serves the connections that arrive at an endpoint, one at a time, with a real
// MessageRequestHandler, until it is stopped
class RequestServer {
public:
   RequestServer(const ListeningEndpoint& endpoint, tonnerre::MessageHandler* handler) :
      m_endpoint(endpoint), m_handler(handler), m_stop(false) {
   }

   void start() {
      m_thread = std::thread([this]() { run(); });
   }

   void stopAndJoin() {
      m_stop = true;
      if (m_thread.joinable()) {
         m_thread.join();
      }
   }

private:
   void run() {
      while (!m_stop) {
         if (!m_endpoint.connectionPending(50)) {
            continue;
         }
         const int fd = ::accept(m_endpoint.fd(), nullptr, nullptr);
         if (fd < 0) {
            continue;
         }
         // the request handler owns the socket and closes it when deleted
         MessageRequestHandler* requestHandler =
            new MessageRequestHandler(new chaudiere::Socket(fd), m_handler);
         requestHandler->run();
         delete requestHandler;
      }
   }

   const ListeningEndpoint& m_endpoint;
   tonnerre::MessageHandler* m_handler;
   std::atomic<bool> m_stop;
   std::thread m_thread;
};

int runResponseDeadlineScenarioInChild(const ReceiveScenario& scenario) {
   // the server's late answer to a connection that the client has already given
   // up on must not be able to kill this process (SIGPIPE is a separate defect)
   ::signal(SIGPIPE, SIG_IGN);

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

   SlowResponseHandler handler(scenario.handlerDelayMillis);
   RequestServer server(endpoint, &handler);
   server.start();

   // 1. a short deadline: the handler is still working when it passes
   {
      Message request("slowRequest", MessageTypeText);
      request.setTextPayload("please wait");
      Message response;

      const std::chrono::steady_clock::time_point began = std::chrono::steady_clock::now();
      const bool received = request.send("t1bService", response, scenario.deadlineMillis);
      const long long elapsed = millisSince(began);
      printf("RESULT short deadline %d ms: send=%s after %lld ms (handler needs %d ms)\n",
             scenario.deadlineMillis, received ? "true" : "false", elapsed, scenario.handlerDelayMillis);

      int result = EXIT_MET;
      if (received) {
         result = EXIT_DEADLINE_OVERRIDE;
      } else if ((response.getType() != MessageTypeUnknown) || !response.getTextPayload().empty()) {
         result = EXIT_STATE_NOT_CLEARED;
      } else {
         result = timingResult(scenario, elapsed);
      }

      if (result == EXIT_MET) {
         // the failed connection must not have gone back to the pool
         chaudiere::Socket* next = messaging->socketForService(info);
         const bool deadSocket = (next == nullptr) || !next->isConnected();
         delete next;
         if (deadSocket) {
            result = EXIT_DEAD_SOCKET_POOLED;
         }
      }

      if (result != EXIT_MET) {
         server.stopAndJoin();
         return result;
      }
   }

   // 2. the longer deadline lets the same operation succeed
   {
      Message request("slowRequest", MessageTypeText);
      request.setTextPayload("please wait");
      Message response;

      const std::chrono::steady_clock::time_point began = std::chrono::steady_clock::now();
      const bool received = request.send("t1bService", response, scenario.longDeadlineMillis);
      const long long elapsed = millisSince(began);
      printf("RESULT long deadline %d ms: send=%s after %lld ms\n",
             scenario.longDeadlineMillis, received ? "true" : "false", elapsed);
      server.stopAndJoin();

      if (!received) {
         return EXIT_DEADLINE_OVERRIDE;
      }
      if ((response.getType() != MessageTypeText) || (response.getTextPayload() != "slow reply")) {
         return EXIT_CONTENT_MISMATCH;
      }
      if (elapsed >= scenario.longDeadlineMillis) {
         return EXIT_TOO_LATE;
      }
   }

   return EXIT_MET;
}

// reconstitute(socket, negative) is a caller's mistake: it is rejected before
// anything is read, and neither the message nor the connection is touched
int runNegativeReconstituteScenarioInChild(const ReceiveScenario& scenario) {
   LoopbackPair pair;
   if (!pair.create()) {
      printf("RESULT setup failed (loopback pair)\n");
      return EXIT_SETUP_FAILED;
   }

   chaudiere::Socket serverSocket(pair.releaseServerFd());
   RawPeer peer(pair.releasePeerFd(), scenario.segments, 0, PeerFinish::Hold);
   peer.start();

   Message message;
   message.setType(MessageTypeText);
   message.setTextPayload("keep me");

   const bool minusOne = message.reconstitute(&serverSocket, -1);
   const bool intMin = message.reconstitute(&serverSocket, INT_MIN);
   const bool untouched = (message.getType() == MessageTypeText) &&
                          (message.getTextPayload() == "keep me") &&
                          serverSocket.isConnected();

   // nothing was consumed: the message that is waiting can still be read
   const bool later = message.reconstitute(&serverSocket, 5000);
   const bool content = later && (message.getTextPayload() == scenario.expectedText);

   serverSocket.close();
   peer.releaseAndJoin();

   printf("RESULT -1 accepted=%s INT_MIN accepted=%s untouched=%s laterRead=%s\n",
          minusOne ? "true" : "false", intMin ? "true" : "false",
          untouched ? "yes" : "no", content ? "yes" : "no");

   return (!minusOne && !intMin && untouched && content) ? EXIT_MET : EXIT_NEGATIVE_MISHANDLED;
}

// send(service, response, negative) is rejected before anything is sent or
// connected
int runNegativeSendScenarioInChild(const ReceiveScenario&) {
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

   Message request("negativeRequest", MessageTypeText);
   request.setTextPayload("never sent");
   Message response;

   const bool minusOne = request.send("t1bService", response, -1);
   const bool intMin = request.send("t1bService", response, INT_MIN);
   const bool connected = endpoint.connectionPending(200);

   printf("RESULT -1 send=%s INT_MIN send=%s connectionMade=%s\n",
          minusOne ? "true" : "false", intMin ? "true" : "false", connected ? "yes" : "no");

   return (!minusOne && !intMin && !connected) ? EXIT_MET : EXIT_NEGATIVE_MISHANDLED;
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
   const ReceiveScenario& scenario = scenarioIn;

   // --scenario <id>: only that scenario (the runner fails if none matched)
   const std::string& onlyScenario = runConfig().scenarioFilter;
   if (!onlyScenario.empty() && (scenario.id != onlyScenario)) {
      return;
   }

   // scenarios that wait out the 10 s production default run only on request
   if (scenario.slowOptIn) {
      const char* slow = std::getenv("TONNERRE_TEST_SLOW");
      if ((slow == nullptr) || (*slow == '\0')) {
         return;
      }
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
         case EXIT_TOO_EARLY:
            problem = "the call gave up before its deadline could have passed";
            break;
         case EXIT_TOO_LATE:
            problem = "the call took far longer than its deadline allows";
            break;
         case EXIT_DEADLINE_OVERRIDE:
            problem = "a response deadline was not honoured (a short one did not fail, or a long one did not allow the response)";
            break;
         case EXIT_NEGATIVE_MISHANDLED:
            problem = "a negative deadline was accepted, or it changed the message, the connection or the stream";
            break;
         case EXIT_NO_SIGNALS:
            problem = "the signal storm delivered no signal, so the scenario proved nothing";
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
      printf("[PASS] %s\n", scenario.id.c_str());

      // TONNERRE_TEST_VERBOSE=1 also shows what a passing scenario observed
      // (for the deadline scenarios: how long the call took)
      const char* verbose = std::getenv("TONNERRE_TEST_VERBOSE");
      if ((verbose != nullptr) && (*verbose != '\0') && !observed.empty()) {
         printf("       observed: %s\n", observed.c_str());
      }
   } else {
      ++tally.unexpectedFailures;
      printf("[FAIL] %s\n", scenario.id.c_str());
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
   testReceiveDeadline();
   testResponseDeadline();
   testDeadlineArguments();
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
// D-8 (fixed in T1c): a peer that connects, sends part of a message and then
// stays silent. The receiver gives up when its deadline passes. These scenarios
// pass a deadline of 500 ms instead of the 10 s default so that they are quick;
// the harness backstop is the deadline plus 5 s, and the elapsed time is checked
// coarsely (at least 90% of the deadline, at most the deadline plus 1.5 s).
//******************************************************************************

namespace {

const int SCENARIO_DEADLINE = 500;

// a Receive scenario that must be rejected at about its deadline
void expectRejectionAtDeadline(ReceiveScenario& s, int deadlineMillis) {
   s.expect = Expect::Reject;
   s.deadlineMillis = deadlineMillis;
   s.minElapsedMillis = deadlineMillis * 9 / 10;
   s.maxElapsedMillis = deadlineMillis + 1500;
   s.timeoutMillis = deadlineMillis + 5000;
}

std::vector<std::string> oneByteSegments(const std::string& wire) {
   std::vector<std::string> segments;
   for (char c : wire) {
      segments.push_back(std::string(1, c));
   }
   return segments;
}

}

void TestReceivePath::testStalledPeer() {
   TEST_CASE("receive path: a stalled peer must not block the receiver indefinitely");

   const std::string wire = validTextWire("hello world");

   {
      ReceiveScenario s;
      s.id = "stalled.silent-after-connect";
      s.description = "peer connects, sends nothing and stays connected";
      s.segments = {};
      s.finish = PeerFinish::Hold;
      expectRejectionAtDeadline(s, SCENARIO_DEADLINE);
      runScenario(s);
   }

   {
      ReceiveScenario s;
      s.id = "stalled.silent-inside-prefix";
      s.description = "peer sends 4 of the 10 prefix bytes and stays connected";
      s.segments = { wire.substr(0, 4) };
      s.finish = PeerFinish::Hold;
      expectRejectionAtDeadline(s, SCENARIO_DEADLINE);
      runScenario(s);
   }

   {
      ReceiveScenario s;
      s.id = "stalled.silent-before-payload";
      s.description = "peer sends the prefix and header but no payload and stays connected";
      s.segments = { wire.substr(0, wire.size() - 11) };
      s.finish = PeerFinish::Hold;
      expectRejectionAtDeadline(s, SCENARIO_DEADLINE);
      runScenario(s);
   }

   {
      ReceiveScenario s;
      s.id = "stalled.silent-inside-header";
      s.description = "peer sends the prefix and 5 header bytes and stays connected";
      s.segments = { wire.substr(0, 15) };
      s.finish = PeerFinish::Hold;
      expectRejectionAtDeadline(s, SCENARIO_DEADLINE);
      runScenario(s);
   }

   {
      ReceiveScenario s;
      s.id = "stalled.silent-inside-payload";
      s.description = "peer sends everything but the last 4 payload bytes and stays connected";
      s.segments = { wire.substr(0, wire.size() - 4) };
      s.finish = PeerFinish::Hold;
      expectRejectionAtDeadline(s, SCENARIO_DEADLINE);
      runScenario(s);
   }
}

//******************************************************************************
// T1c: one deadline for the whole message
//******************************************************************************

void TestReceivePath::testReceiveDeadline() {
   TEST_CASE("receive path: one deadline covers the whole message");

   const std::string wire = validTextWire("hello world");
   const std::string header = textHeader("11");
   const std::string prefix = pad10(std::to_string(header.size()));

   // 4. progress does not extend the deadline
   {
      ReceiveScenario s;
      s.id = "deadline.trickle-is-cut-off";
      s.description = "peer sends one byte every 40 ms, so the message would take over 3 s; "
                      "every gap is far shorter than the 500 ms deadline, but the receiver gives up "
                      "at the deadline";
      s.segments = oneByteSegments(wire);
      s.delayMillis = 40;
      s.finish = PeerFinish::Close;
      expectRejectionAtDeadline(s, SCENARIO_DEADLINE);
      runScenario(s);
   }

   // a slow but complete message inside the deadline is accepted
   {
      ReceiveScenario s;
      s.id = "deadline.slow-trickle-within-deadline-is-accepted";
      s.description = "peer sends one byte every 20 ms (about 1.5 s in all) and the deadline is 6 s";
      const std::string small = validTextWire("");
      s.segments = oneByteSegments(small);
      s.delayMillis = 20;
      s.finish = PeerFinish::Close;
      s.expect = Expect::Accept;
      s.expectedText = "";
      s.deadlineMillis = 6000;
      s.maxElapsedMillis = 5500;
      s.timeoutMillis = 11000;
      runScenario(s);
   }

   // 5. a valid segmented message completed before the deadline
   {
      ReceiveScenario s;
      s.id = "deadline.segmented-message-before-deadline";
      s.description = "a valid message in five segments 40 ms apart, deadline 5 s";
      const std::size_t n = wire.size();
      s.segments = { wire.substr(0, 3), wire.substr(3, 12), wire.substr(15, 20),
                     wire.substr(35, n - 35 - 5), wire.substr(n - 5) };
      s.delayMillis = 40;
      s.finish = PeerFinish::Close;
      s.expect = Expect::Accept;
      s.expectedText = "hello world";
      s.deadlineMillis = 5000;
      s.maxElapsedMillis = 3000;
      s.timeoutMillis = 10000;
      runScenario(s);
   }

   // 6. one deadline for prefix, header and payload together
   {
      ReceiveScenario s;
      s.id = "deadline.spans-prefix-header-payload";
      s.description = "prefix, header and payload arrive 350 ms apart: every read alone is well inside "
                      "the 500 ms deadline, but the message needs 700 ms, so a deadline that restarted "
                      "for each read would accept it";
      s.segments = { prefix, header, "hello world" };
      s.delayMillis = 350;
      s.finish = PeerFinish::Hold;
      expectRejectionAtDeadline(s, SCENARIO_DEADLINE);
      runScenario(s);
   }

   // 7. the deadline starts when reconstitute() begins, not at the first byte
   {
      ReceiveScenario s;
      s.id = "deadline.starts-on-entry";
      s.description = "the peer is silent for 300 ms, then sends the prefix and header, then the "
                      "payload 300 ms later: the message takes 300 ms from its first byte, but 600 ms "
                      "from the start of the call, and the deadline is 500 ms";
      s.segments = { wire.substr(0, wire.size() - 11), wire.substr(wire.size() - 11) };
      s.initialDelayMillis = 300;
      s.delayMillis = 300;
      s.finish = PeerFinish::Hold;
      expectRejectionAtDeadline(s, SCENARIO_DEADLINE);
      runScenario(s);
   }

   // a zero budget is never an unlimited read
   {
      ReceiveScenario s;
      s.id = "deadline.zero-with-silent-peer";
      s.description = "deadline 0 and a silent peer: no waiting at all (not an unlimited wait)";
      s.segments = {};
      s.finish = PeerFinish::Hold;
      s.expect = Expect::Reject;
      s.deadlineMillis = 0;
      s.maxElapsedMillis = 1500;
      s.timeoutMillis = 5000;
      runScenario(s);
   }

   {
      ReceiveScenario s;
      s.id = "deadline.zero-with-partial-message";
      s.description = "deadline 0 and a message that is only partly available: rejected at once";
      s.segments = { wire.substr(0, wire.size() - 1) };
      s.finish = PeerFinish::Hold;
      s.settleMillis = 200;
      s.expect = Expect::Reject;
      s.deadlineMillis = 0;
      s.maxElapsedMillis = 1500;
      s.timeoutMillis = 5000;
      runScenario(s);
   }

   {
      ReceiveScenario s;
      s.id = "deadline.zero-with-complete-message";
      s.description = "deadline 0 and a complete message that is already available: accepted without waiting";
      s.segments = { wire };
      s.finish = PeerFinish::Hold;
      s.settleMillis = 200;
      s.expect = Expect::Accept;
      s.expectedText = "hello world";
      s.deadlineMillis = 0;
      s.maxElapsedMillis = 1500;
      s.timeoutMillis = 5000;
      runScenario(s);
   }

   // a very large deadline must not overflow
   {
      ReceiveScenario s;
      s.id = "deadline.int-max";
      s.description = "deadline INT_MAX with a valid message: accepted at once (no arithmetic overflow)";
      s.segments = { wire };
      s.finish = PeerFinish::Close;
      s.expect = Expect::Accept;
      s.expectedText = "hello world";
      s.deadlineMillis = INT_MAX;
      s.maxElapsedMillis = 3000;
      s.timeoutMillis = 8000;
      runScenario(s);
   }

   // 12. signals interrupting the reception
   {
      ReceiveScenario s;
      s.id = "deadline.signals-do-not-fail-a-valid-message";
      s.description = "a valid message in six segments 40 ms apart while SIGUSR1 interrupts the "
                      "receiving thread every 2 ms";
      const std::size_t n = wire.size();
      s.segments = { wire.substr(0, 4), wire.substr(4, 8), wire.substr(12, 12),
                     wire.substr(24, 12), wire.substr(36, n - 36 - 4), wire.substr(n - 4) };
      s.delayMillis = 40;
      s.finish = PeerFinish::Close;
      s.expect = Expect::Accept;
      s.expectedText = "hello world";
      s.deadlineMillis = 5000;
      s.signalIntervalMillis = 2;
      s.maxElapsedMillis = 4000;
      s.timeoutMillis = 10000;
      runScenario(s);
   }

   {
      ReceiveScenario s;
      s.id = "deadline.signals-do-not-change-the-deadline";
      s.description = "a silent peer while SIGUSR1 interrupts the receiving thread every 2 ms: the "
                      "deadline is neither cut short nor extended";
      s.segments = {};
      s.finish = PeerFinish::Hold;
      s.signalIntervalMillis = 2;
      expectRejectionAtDeadline(s, SCENARIO_DEADLINE);
      runScenario(s);
   }

   // the production default itself (10 s), through reconstitute(socket); it
   // waits out the whole default, so it runs only when TONNERRE_TEST_SLOW is set
   {
      ReceiveScenario s;
      s.id = "deadline.default-applies-to-reconstitute";
      s.description = "reconstitute(socket) with a silent peer gives up at the 10 s default "
                      "(opt-in: set TONNERRE_TEST_SLOW)";
      s.segments = {};
      s.finish = PeerFinish::Hold;
      s.expect = Expect::Reject;
      s.deadlineMillis = -1;
      s.minElapsedMillis = 9000;
      s.maxElapsedMillis = 11500;
      s.timeoutMillis = 20000;
      s.slowOptIn = true;
      runScenario(s);
   }
}

//******************************************************************************
// T1c: the deadline for the response of send(service, response, deadline)
//******************************************************************************

void TestReceivePath::testResponseDeadline() {
   TEST_CASE("receive path: the response deadline of send(service, response, deadline)");

   {
      ReceiveScenario s;
      s.id = "response.deadline-override-with-slow-handler";
      s.description = "the server's handler needs 1200 ms: a 400 ms response deadline fails (and the "
                      "failed connection is not pooled), a 6000 ms one succeeds";
      s.kind = Kind::ResponseDeadline;
      s.handlerDelayMillis = 1200;
      s.deadlineMillis = 400;
      s.longDeadlineMillis = 6000;
      s.minElapsedMillis = 360;
      s.maxElapsedMillis = 400 + 1500;
      s.timeoutMillis = 20000;
      runScenario(s);
   }
}

//******************************************************************************
// T1c: arguments and constants
//******************************************************************************

void TestReceivePath::testDeadlineArguments() {
   TEST_CASE("receive path: the default deadline and the rejection of negative deadlines");

   // the production default is 10 seconds; it is not shortened for the tests
   static_assert(Message::DEFAULT_RECEIVE_DEADLINE_MILLIS == 10000,
                 "the default receive deadline is 10,000 ms");
   require(Message::DEFAULT_RECEIVE_DEADLINE_MILLIS == 10000,
           "the default receive deadline is 10,000 ms");

   {
      ReceiveScenario s;
      s.id = "deadline.negative-reconstitute-is-rejected";
      s.description = "reconstitute(socket, -1) and (socket, INT_MIN) return false without touching the "
                      "message, the socket or the stream";
      s.kind = Kind::NegativeReconstituteDeadline;
      s.segments = { validTextWire("still waiting") };
      s.expectedText = "still waiting";
      s.timeoutMillis = 15000;
      runScenario(s);
   }

   {
      ReceiveScenario s;
      s.id = "deadline.negative-send-is-rejected";
      s.description = "send(service, response, -1) and (.., INT_MIN) return false and connect to nothing";
      s.kind = Kind::NegativeSendDeadline;
      s.timeoutMillis = 15000;
      runScenario(s);
   }
}
