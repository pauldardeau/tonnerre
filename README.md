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

Configuration File
------------------
Tonnerre uses an .INI for configuration. See 'What's an INI file'
if you're not familiar with them. The .INI format was chosen
because it's very simple and I like simple.

Platforms/Compilers
-------------------
This project is implemented in c++ (pre-c++11). It is tested with g++
compiler on Linux, FreeBSD, and OpenBSD.

Status/Maturity
---------------
Parts of the code are very mature and well tested, and other parts
aren't. I wouldn't use this for anything important just yet. It'll
slowly get there.

C++ Standards
-------------
The code is intentionally pre-c++11 to provide for maximum portability.

Coding Style
------------
You may find my coding style slightly peculiar. It's my personal style
that I've adopted after a lot of experimentation about what works best
for me.

What's An INI File
------------------
In the 'old days', Windows computers made extensive use of INI files.
This was before the system registry came along. An INI file is a
simple text file that is composed of sections, and each section
can have keys and values.

Meaning of Tonnerre
-------------------
What does 'Tonnerre' mean?  It's a French word that means "thunder" or
"thunder clap". My grandfather used to frequently say "Tonnerre!" as
an expression of astonishment (similar to how people say "Really!").

