// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef TONNERRE_TEST_LOOPBACKCONNECTION_H
#define TONNERRE_TEST_LOOPBACKCONNECTION_H

#include "ServerSocket.h"
#include "Socket.h"

namespace tonnerre_test {

/**
 * LoopbackConnection sets up a real TCP connection on 127.0.0.1 for tests
 * that need an actual working Socket pair (there's no MockSocket in
 * tonnerre's tests). Binds/listens on the given port, connects a client
 * Socket to it, and accepts the corresponding server-side Socket.
 */
struct LoopbackConnection {
   chaudiere::ServerSocket serverListener;
   chaudiere::Socket* clientSocket;
   chaudiere::Socket* serverSideSocket;

   explicit LoopbackConnection(int port) :
      serverListener(port),
      clientSocket(new chaudiere::Socket("127.0.0.1", port)),
      serverSideSocket(serverListener.accept()) {
   }

   ~LoopbackConnection() {
      delete clientSocket;
      delete serverSideSocket;
   }

   // no copies
   LoopbackConnection(const LoopbackConnection&) = delete;
   LoopbackConnection& operator=(const LoopbackConnection&) = delete;
};

}

#endif
