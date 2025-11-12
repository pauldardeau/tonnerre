[![Build and test](https://github.com/pauldardeau/tonnerre/actions/workflows/build-and-test-cpp.yml/badge.svg)](https://github.com/pauldardeau/tonnerre/actions/workflows/build-and-test-cpp.yml)

tonnerre
========

Tonnerre provides simple messaging between applications. By default, the
messages are transferred using TCP/IP sockets. As such, the sender and
receiver can be on the same or different machines. 2 types are supported
for the message payload: key/value pairs or raw strings. Raw string payloads
are useful for transferring JSON or XML.

License
-------
BSD

Dependencies
------------
Chaudière - C++ core functionality with sockets, threading, configuration, etc.
Poivre - c++ unit tests

Configuration File
------------------
Tonnerre uses an .INI for configuration. The .INI format was chosen
because it's very simple and I like simple.

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

