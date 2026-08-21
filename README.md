[![Build and test](https://github.com/pauldardeau/tonnerre/actions/workflows/build-and-test-cpp.yml/badge.svg)](https://github.com/pauldardeau/tonnerre/actions/workflows/build-and-test-cpp.yml)

tonnerre
========

Tonnerre provides simple messaging between applications. By default, the
messages are transferred using TCP/IP sockets. As such, the sender and
receiver can be on the same or different machines. 2 types are supported
for the message payload: key/value pairs or raw strings. Raw string payloads
are useful for transferring JSON or XML.

A "service" is just a name registered in a config file, mapped to a host
and port. A process can be a client (send a `Message` to a service and,
optionally, wait for a response), a server (register a `MessageHandler`
and listen for incoming messages for one service), or both.

License
-------
BSD

Dependencies
------------
Chaudière - C++ core functionality with sockets, threading, configuration, etc.
Poivre - C++ unit tests

Both are included as git submodules (`chaudiere/`, and `chaudiere/poivre/`
nested inside it). Clone with `--recurse-submodules`, or after cloning:

```bash
git submodule update --init --recursive
```

Building
--------
```bash
make -C chaudiere/src   # builds libchaudiere.so
make -C src             # builds tonnerre.so
make -C test            # builds test_tonnerre, TestClient, TestServer
```

Your own programs need `-I` for both `src/` and `chaudiere/src/`, and link
against `tonnerre.so` and `chaudiere/src/libchaudiere.so` (see `test/Makefile`
for the exact flags).

### Building with CMake

A `CMakeLists.txt` is also provided:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure   # builds and runs test_tonnerre
```

To depend on tonnerre from another CMake project - vendored as a git submodule:

```cmake
add_subdirectory(tonnerre)
target_link_libraries(my_target PRIVATE tonnerre)
```

No `-I` needed - the include directory, chaudière (linked transitively), and C++20 all
propagate automatically. The Makefile isn't going anywhere; both build systems compile the
same sources.

Configuration File
------------------
Tonnerre uses an .INI for configuration. The .INI format was chosen
because it's very simple and I like simple.

Every process using tonnerre — client or server — needs a `[services]`
section listing the services it knows about, each mapped to a section name
with that service's connection details:

```ini
[services]
echo_service = EchoService

[EchoService]
host = 127.0.0.1
port = 9000
persistent = true
```

- `host` / `port` — where a client connects to reach this service, and
  where a server providing this service should listen.
- `persistent` (optional, defaults to false) — keep the client-side
  connection open and reuse it for later sends to this service, instead of
  opening a new connection per message.

A process that's *hosting* a service (see `MessagingServer` below) can
also add a `[server]` section to control how it listens:

```ini
[server]
port = 9000
threading = pthreads
```

If `[server]` is omitted, the server listens on chaudière's default port
(9000) using the `pthreads` threading model. `threading` may also be set
to `none` to handle requests synchronously on the accept thread instead of
dispatching them to a thread pool.

Sending a Message (Client)
---------------------------
Call `Messaging::initialize()` once with your config file, then construct
and send `Message` objects. There are two payload types:

```cpp
#include "Messaging.h"
#include "Message.h"
#include "KeyValuePairs.h"

using namespace tonnerre;
using namespace chaudiere;

Messaging::initialize("tonnerre.ini");

// key/value payload, waiting for a response
KeyValuePairs kvp;
kvp.addPair("firstName", "Mickey");
kvp.addPair("lastName", "Mouse");

Message request("echo", MessageTypeKeyValues);
request.setKeyValuesPayload(kvp);

Message response;
if (request.send("echo_service", response)) {
   const KeyValuePairs& responseKvp = response.getKeyValuesPayload();
   // ... use responseKvp ...
} else {
   // unable to deliver the message or read a response
}
```

- The first `Message` constructor argument (`"echo"` above) is the request
  name — it's up to your `MessageHandler` to interpret it, similar to an
  RPC method name or an HTTP route.
- `MessageType` is `MessageTypeText` or `MessageTypeKeyValues`, matching
  whether you call `setTextPayload()`/`getTextPayload()` or
  `setKeyValuesPayload()`/`getKeyValuesPayload()`.
- `send(serviceName, responseMessage)` blocks for a response.
  `send(serviceName)` (no response argument) fires the message and doesn't
  wait for one.

Receiving Messages (Server)
-----------------------------
Implement `MessageHandlerAdapter` (a `MessageHandler` with no-op defaults —
only override the payload type(s) you actually handle), then run a
`MessagingServer` for the service you're providing:

```cpp
#include "MessagingServer.h"
#include "MessageHandlerAdapter.h"
#include "Message.h"
#include "KeyValuePairs.h"

using namespace tonnerre;
using namespace chaudiere;

class EchoHandler : public MessageHandlerAdapter {
public:
   void handleKeyValuesMessage(const Message& requestMessage,
                                Message& responseMessage,
                                const std::string& requestName,
                                const KeyValuePairs& requestPayload,
                                KeyValuePairs& responsePayload) override {
      if (requestName == "echo") {
         responsePayload = requestPayload;
      }
   }
};

int main() {
   EchoHandler handler;

   // 2nd arg is which registered service (from the .ini) this process
   // is providing -- used to determine what host/port to listen on.
   MessagingServer server("tonnerre.ini", "echo_service", &handler);
   server.run();  // blocks, servicing requests until the process exits
}
```

`MessageHandler` also has `handleTextMessage()` for `MessageTypeText`
requests, with the same shape (request/response `Message`s, request name,
and `std::string` payloads instead of `KeyValuePairs`).

See `test/TestClient.cpp` and `test/TestServer.cpp` for complete,
runnable versions of both sides (including a text-payload example and a
service with no request payload), and `test/tonnerre.ini` for a
multi-service config file.

Platforms/Compilers
-------------------
This project is implemented in c++20. It is tested on Linux (Ubuntu,
Debian, and Mint), FreeBSD, and OpenBSD.

C++ Standards
-------------
The code is targeted for c++20.

Meaning of Tonnerre
-------------------
What does 'Tonnerre' mean?  It's a French word that means "thunder" or
"thunder clap". My grandfather used to frequently say "Tonnerre!" as
an expression of astonishment (similar to how people say "Really!").
