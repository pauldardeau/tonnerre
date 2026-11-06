// Copyright Paul Dardeau, SwampBits LLC 2026
// BSD License

#ifndef TONNERRE_TEST_LOOPBACKPAIR_H
#define TONNERRE_TEST_LOOPBACKPAIR_H

#include <cstring>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace tonnerre_test {

/**
 * Both ends set SO_REUSEADDR: the established suites listen on fixed ports that
 * lie inside the kernel's ephemeral range, and a lingering TIME_WAIT left by a
 * client socket without SO_REUSEADDR that happened to be assigned one of those
 * ports would make their bind() fail with EADDRINUSE for about a minute.
 *
 * A connected TCP pair on 127.0.0.1 using an ephemeral port chosen by the
 * kernel (no fixed port, nothing outside loopback). serverFd is the accepted
 * end (to be handed to chaudiere::Socket); peerFd is the connecting end that
 * a test drives with raw send() calls. The listening socket is closed as soon
 * as the connection is accepted. POSIX only.
 */
struct LoopbackPair {
   int serverFd = -1;
   int peerFd = -1;

   static void allowAddressReuse(int fd) {
      const int on = 1;
      ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
   }

   bool create() {
      const int listenFd = ::socket(AF_INET, SOCK_STREAM, 0);
      if (listenFd < 0) {
         return false;
      }
      allowAddressReuse(listenFd);

      sockaddr_in addr;
      std::memset(&addr, 0, sizeof(addr));
      addr.sin_family = AF_INET;
      addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
      addr.sin_port = 0;

      socklen_t addrLen = sizeof(addr);
      if ((::bind(listenFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) ||
          (::listen(listenFd, 1) != 0) ||
          (::getsockname(listenFd, reinterpret_cast<sockaddr*>(&addr), &addrLen) != 0)) {
         ::close(listenFd);
         return false;
      }

      peerFd = ::socket(AF_INET, SOCK_STREAM, 0);
      if (peerFd < 0) {
         ::close(listenFd);
         return false;
      }
      allowAddressReuse(peerFd);

      // the connection completes against the listen backlog before accept()
      if (::connect(peerFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
         ::close(listenFd);
         ::close(peerFd);
         peerFd = -1;
         return false;
      }

      serverFd = ::accept(listenFd, nullptr, nullptr);
      ::close(listenFd);

      if (serverFd < 0) {
         ::close(peerFd);
         peerFd = -1;
         return false;
      }

      return true;
   }

   // hands ownership of the peer end to the caller
   int releasePeerFd() {
      const int fd = peerFd;
      peerFd = -1;
      return fd;
   }

   // hands ownership of the server end to the caller
   int releaseServerFd() {
      const int fd = serverFd;
      serverFd = -1;
      return fd;
   }

   ~LoopbackPair() {
      if (serverFd >= 0) {
         ::close(serverFd);
      }
      if (peerFd >= 0) {
         ::close(peerFd);
      }
   }
};

}

#endif
