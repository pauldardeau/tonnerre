// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#include <limits>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Message.h"
#include "Logger.h"
#include "StrUtils.h"
#include "Socket.h"
#include "Messaging.h"

using namespace std;
using namespace chaudiere;
using namespace tonnerre;


static const std::string EMPTY_STRING           = "";

// largest header and largest payload that will be sent or accepted; both limits
// are enforced identically when sending and when receiving
static const std::size_t MAX_HEADER_LENGTH      = 32767;
static const std::size_t MAX_PAYLOAD_LENGTH     = 32767;

static const int NUM_CHARS_HEADER_LENGTH        = 10;

static const std::string DELIMITER_KEY_VALUE    = "=";
static const std::string DELIMITER_PAIR         = ";";

static const std::string KEY_ONE_WAY            = "1way";
static const std::string KEY_PAYLOAD_LENGTH     = "payload_length";
static const std::string KEY_PAYLOAD_TYPE       = "payload_type";
static const std::string KEY_REQUEST_NAME       = "request";

static const std::string VALUE_PAYLOAD_KVP      = "kvp";
static const std::string VALUE_PAYLOAD_TEXT     = "text";
static const std::string VALUE_PAYLOAD_UNKNOWN  = "unknown";
static const std::string VALUE_TRUE             = "true";

using namespace chaudiere;
using namespace tonnerre;

//******************************************************************************

namespace {

// Parses a length that arrived over the wire: one or more ASCII digits and
// nothing else (no sign, blanks, radix prefix, exponent or decimal point).
// Returns false, leaving 'value' untouched, for anything else and for values
// that do not fit in a std::size_t. Never throws.
bool parseWireLength(const std::string& text, std::size_t& value) {
   if (text.empty()) {
      return false;
   }

   const std::size_t maxValue = std::numeric_limits<std::size_t>::max();
   std::size_t result = 0;

   for (const char c : text) {
      if ((c < '0') || (c > '9')) {
         return false;
      }

      const std::size_t digit = static_cast<std::size_t>(c - '0');
      if (result > ((maxValue - digit) / 10)) {
         return false;
      }
      result = (result * 10) + digit;
   }

   value = result;
   return true;
}

// Reads exactly 'count' bytes from the socket. Socket::readSocket() returns
// the number of bytes actually received, which is short when the peer closes
// or the connection fails mid-read (and -1 when nothing could be read), so the
// read succeeded only if that number equals the number requested. The caller
// must already have checked 'count' against a protocol limit; the check here
// only guarantees the count can be handed to readSocket() as an int.
bool readExact(Socket* socket, std::size_t count, std::string& bytes) {
   bytes.clear();

   if (count == 0) {
      return true;
   }

   if (count > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
      return false;
   }

   std::string buffer(count, '\0');
   const int requested = static_cast<int>(count);
   const int received = socket->readSocket(&buffer[0], requested);

   if (received != requested) {
      return false;
   }

   bytes.swap(buffer);
   return true;
}

// Strictly parses a flattened header ("key=value;key=value"). Every pair must
// be exactly "key=value" with a non-empty key and a value that contains no
// further '='; empty pairs and repeated keys are malformed. A pair with an
// empty value ("request=") is what the sender writes for an empty value and
// is accepted, but, as before, is not stored.
bool parseHeaders(const std::string& text,
                  KeyValuePairs& headers,
                  std::string& problem) {
   std::set<std::string> keysSeen;
   std::size_t position = 0;

   while (true) {
      const std::size_t end = text.find(DELIMITER_PAIR, position);
      const std::string pair =
         text.substr(position,
                     (end == std::string::npos) ? std::string::npos : end - position);

      const std::size_t equals = pair.find(DELIMITER_KEY_VALUE);
      if (equals == std::string::npos) {
         problem = "header pair has no '='";
         return false;
      }

      const std::string key = pair.substr(0, equals);
      const std::string value = pair.substr(equals + 1);

      if (key.empty()) {
         problem = "header pair has an empty key";
         return false;
      }

      if (value.find(DELIMITER_KEY_VALUE) != std::string::npos) {
         problem = "header pair has more than one '='";
         return false;
      }

      if (!keysSeen.insert(key).second) {
         problem = "header key '" + key + "' is repeated";
         return false;
      }

      if (!value.empty()) {
         headers.addPair(key, value);
      }

      if (end == std::string::npos) {
         break;
      }
      position = end + 1;
   }

   return true;
}

}

//******************************************************************************

tonnerre::Message* Message::reconstruct(Socket* socket) {
   Message* message = new Message();
   if (message->reconstitute(socket)) {
      return message;
   } else {
      delete message;
      return nullptr;
   }
}

//******************************************************************************

Message::Message() :
   m_messageType(MessageTypeUnknown),
   m_isOneWay(false),
   m_persistentConnection(false) {
   Logger::logInstanceCreate("Message");
}

//******************************************************************************

Message::Message(const std::string& requestName, MessageType messageType) :
   m_messageType(messageType),
   m_isOneWay(false),
   m_persistentConnection(false) {
   Logger::logInstanceCreate("Message");
   m_kvpHeaders.addPair(KEY_REQUEST_NAME, requestName);
}

//******************************************************************************

Message::Message(const Message& copy) :
   m_serviceName(copy.m_serviceName),
   m_textPayload(copy.m_textPayload),
   m_kvpPayload(copy.m_kvpPayload),
   m_kvpHeaders(copy.m_kvpHeaders),
   m_messageType(copy.m_messageType),
   m_isOneWay(copy.m_isOneWay),
   m_persistentConnection(false) {
   Logger::logInstanceCreate("Message");
}

//******************************************************************************

Message::~Message() {
   Logger::logInstanceDestroy("Message");
}

//******************************************************************************

Message& Message::operator=(const Message& copy) {
   if (this == &copy) {
      return *this;
   }

   m_serviceName = copy.m_serviceName;
   m_textPayload = copy.m_textPayload;
   m_kvpPayload = copy.m_kvpPayload;
   m_kvpHeaders = copy.m_kvpHeaders;
   m_messageType = copy.m_messageType;
   m_isOneWay = copy.m_isOneWay;
   m_persistentConnection = false;

   return *this;
}

//******************************************************************************

bool Message::send(const std::string& serviceName) {
   if (m_messageType == MessageTypeUnknown) {
      Logger::error("unable to send message, no message type set");
      return false;
   }

   m_isOneWay = true;

   std::string wireMessage;
   if (!toWireString(wireMessage)) {
      Logger::error("unable to send message, it exceeds the maximum message size");
      return false;
   }

   Socket* socket(socketForService(serviceName));

   if (socket != nullptr) {
      if (socket->write(wireMessage)) {
         returnSocketForService(serviceName, socket);
         return true;
      } else {
         // unable to write to socket; part of the message may have been
         // written, so the stream is no longer at a message boundary and the
         // connection must not be reused
         Logger::error("unable to write to socket");
      }

      delete socket;
   } else {
      // unable to connect to service
      Logger::error("unable to connect to service");
   }

   return false;
}

//******************************************************************************

bool Message::send(const std::string& serviceName, Message& responseMessage) {
   if (m_messageType == MessageTypeUnknown) {
      Logger::error("unable to send message, no message type set");
      return false;
   }

   std::string wireMessage;
   if (!toWireString(wireMessage)) {
      Logger::error("unable to send message, it exceeds the maximum message size");
      return false;
   }

   Socket* socket(socketForService(serviceName));

   if (socket != nullptr) {
      if (socket->write(wireMessage)) {
         // a failed reconstitute() closes the connection, and
         // returnSocketForService() does not pool a closed socket
         const bool rc = responseMessage.reconstitute(socket);
         returnSocketForService(serviceName, socket);
         return rc;
      } else {
         // unable to write to socket; part of the message may have been
         // written, so the stream is no longer at a message boundary and the
         // connection must not be reused
         Logger::error("unable to write to socket");
      }

      delete socket;
   } else {
      // unable to connect to service
      Logger::error("unable to connect to service");
   }

   return false;
}

//******************************************************************************

void Message::setType(MessageType messageType) {
   m_messageType = messageType;
}

//******************************************************************************

MessageType Message::getType() const {
   return m_messageType;
}

//******************************************************************************

const KeyValuePairs& Message::getKeyValuesPayload() const {
   return m_kvpPayload;
}

//******************************************************************************

const std::string& Message::getTextPayload() const {
   return m_textPayload;
}

//******************************************************************************

void Message::setKeyValuesPayload(const KeyValuePairs& kvp) {
   m_kvpPayload = kvp;
}

//******************************************************************************

void Message::setTextPayload(const std::string& text) {
   m_textPayload = text;
}

//******************************************************************************

const std::string& Message::getServiceName() const {
   return m_serviceName;
}

//******************************************************************************

std::string Message::getRequestName() const {
   if (m_kvpHeaders.hasKey(KEY_REQUEST_NAME)) {
      return std::string(m_kvpHeaders.getValue(KEY_REQUEST_NAME));
   } else {
      return EMPTY_STRING;
   }
}

//******************************************************************************

Socket* Message::socketForService(const std::string& serviceName) const {
   std::shared_ptr<Messaging> messaging(Messaging::getMessaging());

   if (messaging != nullptr) {
      if (messaging->isServiceRegistered(serviceName)) {
         const ServiceInfo& serviceInfo =
            messaging->getInfoForService(serviceName);
         m_persistentConnection = serviceInfo.getPersistentConnection();

         //const std::string& host = serviceInfo.host();
         //const unsigned short port = serviceInfo.port();
         //printf("host='%s'\n", host.c_str());
         //printf("port='%d'\n", port);

         return messaging->socketForService(serviceInfo);
      } else {
         Logger::error("service is not registered");
         printf("service is not registered\n");
      }
   } else {
      Logger::error("messaging not initialized");
   }

   return nullptr;
}

//******************************************************************************

void Message::returnSocketForService(const std::string& serviceName,
                                     chaudiere::Socket* socket) {
   if (socket == nullptr) {
      return;
   }

   // a closed socket (reconstitute() closes the connection when framing fails)
   // is never pooled
   if (m_persistentConnection && !serviceName.empty() && socket->isConnected()) {
      std::shared_ptr<Messaging> messaging(Messaging::getMessaging());
      if (messaging != nullptr) {
         if (messaging->isServiceRegistered(serviceName)) {
            const ServiceInfo& serviceInfo =
               messaging->getInfoForService(serviceName);
            if (serviceInfo.getPersistentConnection()) {
               messaging->returnSocketForService(serviceInfo, socket);
               return;
            }
         }
      }
   }

   // not a persistent connection (or unable to pool it) -- close it
   // rather than leaking the fd
   delete socket;
}

//******************************************************************************

std::string Message::readSocketBytes(Socket* socket,
                                     int numberBytes,
                                     bool& success) {
   std::string bytes;

   success = (socket != nullptr) &&
             (numberBytes >= 0) &&
             readExact(socket, static_cast<std::size_t>(numberBytes), bytes);

   if (!success) {
      Logger::error("reading socket failed, the requested bytes did not arrive");
      bytes.clear();
   }

   return bytes;
}

//******************************************************************************

void Message::clearReceivedState() {
   m_kvpHeaders.clear();
   m_textPayload.clear();
   m_kvpPayload.clear();
   m_messageType = MessageTypeUnknown;
   m_isOneWay = false;
}

//******************************************************************************

bool Message::reconstitute(Socket* socket) {
   if (socket == nullptr) {
      // no socket given
      Logger::error("no socket given to reconstitute");
      return false;
   }

   // Everything is read and validated into locals first; the message is
   // modified only once the complete message has been received.
   KeyValuePairs headers;
   KeyValuePairs kvpPayload;
   std::string textPayload;
   MessageType messageType = MessageTypeUnknown;
   bool isOneWay = false;
   std::string problem;

   do {
      // header-length prefix: exactly 10 characters, a decimal number padded
      // on the right with spaces
      std::string prefix;
      if (!readExact(socket, NUM_CHARS_HEADER_LENGTH, prefix)) {
         problem = "header-length prefix incomplete";
         break;
      }

      StrUtils::stripTrailing(prefix, ' ');

      std::size_t headerLength = 0;
      if (!parseWireLength(prefix, headerLength)) {
         problem = "header-length prefix is not a valid decimal length";
         break;
      }

      if (headerLength == 0) {
         problem = "header length is empty";
         break;
      }

      if (headerLength > MAX_HEADER_LENGTH) {
         problem = "header length " + StrUtils::size_tToString(headerLength) +
                   " exceeds the maximum of " + StrUtils::size_tToString(MAX_HEADER_LENGTH);
         break;
      }

      std::string headerAsString;
      if (!readExact(socket, headerLength, headerAsString)) {
         problem = "header incomplete";
         break;
      }

      if (!parseHeaders(headerAsString, headers, problem)) {
         break;
      }

      // payload type: required, and must be one that is supported
      if (!headers.hasKey(KEY_PAYLOAD_TYPE)) {
         problem = "header has no payload_type";
         break;
      }

      const std::string& valuePayloadType = headers.getValue(KEY_PAYLOAD_TYPE);
      if (valuePayloadType == VALUE_PAYLOAD_TEXT) {
         messageType = MessageTypeText;
      } else if (valuePayloadType == VALUE_PAYLOAD_KVP) {
         messageType = MessageTypeKeyValues;
      } else {
         problem = "unrecognized payload type";
         break;
      }

      // payload length: required, valid, and within the limit *before* any
      // payload bytes are read, so an oversized payload is never consumed (or
      // dropped) and never left behind to be mistaken for the next message
      if (!headers.hasKey(KEY_PAYLOAD_LENGTH)) {
         problem = "header has no payload_length";
         break;
      }

      std::size_t payloadLength = 0;
      if (!parseWireLength(headers.getValue(KEY_PAYLOAD_LENGTH), payloadLength)) {
         problem = "payload_length is not a valid decimal length";
         break;
      }

      if (payloadLength > MAX_PAYLOAD_LENGTH) {
         problem = "payload length " + StrUtils::size_tToString(payloadLength) +
                   " exceeds the maximum of " + StrUtils::size_tToString(MAX_PAYLOAD_LENGTH);
         break;
      }

      std::string payloadAsString;
      if (!readExact(socket, payloadLength, payloadAsString)) {
         problem = "payload incomplete";
         break;
      }

      if (messageType == MessageTypeText) {
         textPayload = payloadAsString;
      } else {
         // the key/values payload encoding is lossy and tolerant of pairs it
         // cannot parse (see T0 defect D-5); it is not validated here
         fromString(payloadAsString, kvpPayload);
      }

      if (headers.hasKey(KEY_ONE_WAY) &&
          (headers.getValue(KEY_ONE_WAY) == VALUE_TRUE)) {
         // mark it as being a 1-way message
         isOneWay = true;
      }

      // complete and valid: replace this message's state
      m_kvpHeaders = headers;
      m_kvpPayload = kvpPayload;
      m_textPayload = textPayload;
      m_messageType = messageType;
      m_isOneWay = isOneWay;
      return true;

   } while (false);

   // The bytes that follow the point of failure cannot be trusted to start a
   // message, so the connection is closed rather than left readable, and the
   // message is left empty rather than partly populated.
   Logger::error("unable to reconstitute message: " + problem);
   clearReceivedState();
   socket->close();
   return false;
}

//******************************************************************************

std::string Message::toString() const {
   std::string messageAsString;
   toWireString(messageAsString);
   return messageAsString;
}

//******************************************************************************

bool Message::toWireString(std::string& messageAsString) const {
   KeyValuePairs kvpHeaders(m_kvpHeaders);
   std::string payload;

   if (m_messageType == MessageTypeText) {
      kvpHeaders.addPair(KEY_PAYLOAD_TYPE, VALUE_PAYLOAD_TEXT);
      payload = m_textPayload;
   } else if (m_messageType == MessageTypeKeyValues) {
      kvpHeaders.addPair(KEY_PAYLOAD_TYPE, VALUE_PAYLOAD_KVP);
      payload = toString(m_kvpPayload);
   } else {
      kvpHeaders.addPair(KEY_PAYLOAD_TYPE, VALUE_PAYLOAD_UNKNOWN);
   }

   if (m_isOneWay) {
      kvpHeaders.addPair(KEY_ONE_WAY, VALUE_TRUE);
   }

   if (m_kvpHeaders.hasKey(KEY_REQUEST_NAME)) {
      kvpHeaders.addPair(KEY_REQUEST_NAME,
                         m_kvpHeaders.getValue(KEY_REQUEST_NAME));
   } else {
      kvpHeaders.addPair(KEY_REQUEST_NAME, EMPTY_STRING);
   }

   const std::size_t payloadLength = payload.length();
   kvpHeaders.addPair(KEY_PAYLOAD_LENGTH, encodeLength(payloadLength));

   const std::string headersAsString = toString(kvpHeaders);

   std::string headerLengthPrefix = encodeLength(headersAsString.length());
   StrUtils::padRight(headerLengthPrefix, ' ', NUM_CHARS_HEADER_LENGTH);

   messageAsString.clear();
   messageAsString += headerLengthPrefix;
   messageAsString += headersAsString;
   messageAsString += payload;

   // a receiver rejects anything larger, so the sender must not send it
   return (headersAsString.length() <= MAX_HEADER_LENGTH) &&
          (payloadLength <= MAX_PAYLOAD_LENGTH);
}

//******************************************************************************

std::string Message::toString(const KeyValuePairs& kvp) {
   std::string kvpAsString;

   if (!kvp.empty()) {
      vector<string> keys;
      kvp.getKeys(keys);
      bool is_first = true;

      for (const auto& key : keys) {
         if (!is_first) {
            // append pair delimiter
            kvpAsString += DELIMITER_PAIR;
         }

         kvpAsString += key;
         kvpAsString += DELIMITER_KEY_VALUE;
         kvpAsString += kvp.getValue(key);

         is_first = false;
      }
   }

   return kvpAsString;
}

//******************************************************************************

bool Message::fromString(const std::string& s, KeyValuePairs& kvp) {
   int numPairsAdded = 0;

   if (!s.empty()) {
      std::vector<std::string> vecPairs = StrUtils::split(s, DELIMITER_PAIR);
      const std::size_t numPairs = vecPairs.size();

      if (numPairs > 0) {
         for (const std::string& keyValuePair : vecPairs) {
            std::vector<std::string> vecKeyValue =
               StrUtils::split(keyValuePair, DELIMITER_KEY_VALUE);
            const std::size_t numTokens = vecKeyValue.size();
            if (numTokens == 2) {
               const std::string& key = vecKeyValue[0];
               const std::string& value = vecKeyValue[1];
               kvp.addPair(key, value);
               ++numPairsAdded;
            }
         }
      }
   }

   return numPairsAdded > 0;
}

//******************************************************************************

std::string Message::encodeLength(std::size_t lengthBytes) {
   return StrUtils::size_tToString(lengthBytes);
}

//******************************************************************************

std::size_t Message::decodeLength(Socket* socket) {
   std::size_t lengthBytes = 0;

   if (socket != nullptr) {
      char lengthAsChars[11];
      memset(lengthAsChars, 0, NUM_CHARS_HEADER_LENGTH+1);
      if (socket->read(lengthAsChars, NUM_CHARS_HEADER_LENGTH)) {
         std::string lengthPrefix(lengthAsChars, NUM_CHARS_HEADER_LENGTH);
         StrUtils::stripTrailing(lengthPrefix, ' ');
         // malformed text decodes as 0 rather than throwing
         parseWireLength(lengthPrefix, lengthBytes);
      }
   }

   return lengthBytes;
}

//******************************************************************************

void Message::setHeader(const std::string& key, const std::string& value) {
   m_kvpHeaders.addPair(key, value);
}

//******************************************************************************

bool Message::hasHeader(const std::string& key) const {
   return m_kvpHeaders.hasKey(key);
}

//******************************************************************************

const std::string& Message::getHeader(const std::string& key) const {
   return m_kvpHeaders.getValue(key);
}

//******************************************************************************
