// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#include <string>
#include <vector>
#include <cstdlib>
#include <string.h>

#include "Message.h"
#include "Logger.h"
#include "StrUtils.h"
#include "Socket.h"
#include "StringTokenizer.h"
#include "Messaging.h"

static const std::string EMPTY_STRING           = "";

static const int MAX_STACK_BUFFER_SIZE          = 4096;
static const int MAX_SEGMENT_LENGTH             = 32767;

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

tonnerre::Message* Message::reconstruct(Socket* socket) {
   Message* message(new Message());
   if (message->reconstitute(socket)) {
      return message;
   } else {
      delete message;
      return NULL;
   }
}

//******************************************************************************

Message::Message() :
   m_messageType(MessageType::Unknown),
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
   if (m_messageType == MessageType::Unknown) {
      Logger::error("unable to send message, no message type set");
      return false;
   }

   Socket* socket = socketForService(serviceName);
   
   if (socket != NULL) {
      m_isOneWay = true;
      
      if (socket->write(toString())) {
         returnSocketForService(serviceName, socket);
         return true;
      } else {
         // unable to write to socket
         Logger::error("unable to write to socket");
      }
      
      returnSocketForService(serviceName, socket);
   } else {
      // unable to connect to service
      Logger::error("unable to connect to service (socketForService returned NULL)");
   }
   
   return false;
}

//******************************************************************************

bool Message::send(const std::string& serviceName, Message& responseMessage) {
   if (m_messageType == MessageType::Unknown) {
      Logger::error("unable to send message, no message type set");
      return false;
   }
   
   Socket* socket(socketForService(serviceName));
   
   if (socket != NULL) {
      //const std::string payload = toString();
      //printf("payload: '%s'\n", payload.c_str());
      
      if (socket->write(toString())) {
         const bool rc = responseMessage.reconstitute(socket);
         returnSocketForService(serviceName, socket);
         return rc;
      } else {
         // unable to write to socket
         Logger::error("unable to write to socket");
      }
      
      returnSocketForService(serviceName, socket);
   } else {
      // unable to connect to service
      Logger::error("unable to connect to service");
   }

   return false;
}

//******************************************************************************

Message::MessageType Message::getType() const {
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
   Messaging* messaging = Messaging::getMessaging();
   
   if (messaging != NULL) {
      if (messaging->isServiceRegistered(serviceName)) {
         const ServiceInfo& serviceInfo = messaging->getInfoForService(serviceName);
         //const std::string& host = serviceInfo.host();
         //const unsigned short port = serviceInfo.port();
         const bool persistentConnection = serviceInfo.getPersistentConnection();

         Socket* socket = NULL;
            
         if (persistentConnection) {
            printf("persistent connection set true\n");
            m_persistentConnection = true;
            socket =
               messaging->socketForService(serviceInfo);
         } else {
            printf("persistent connection set false\n");
            socket = messaging->socketForService(serviceInfo);
         }
            
         return socket;
      } else {
         Logger::error("service is not registered");
      }
   } else {
      Logger::error("messaging not initialized");
   }
   
   return NULL;
}

//******************************************************************************

void Message::returnSocketForService(const std::string& serviceName,
                                     chaudiere::Socket* socket) {
   if (m_persistentConnection) {
      if (!serviceName.empty() && (socket != NULL)) {
         Messaging* messaging(Messaging::getMessaging());
         if (messaging != NULL) {
            if (messaging->isServiceRegistered(serviceName)) {
               const ServiceInfo& serviceInfo = messaging->getInfoForService(serviceName);
               if (serviceInfo.getPersistentConnection()) {
                  messaging->returnSocketForService(serviceInfo, socket);
               }
            }
         }
      }
   }
}

//******************************************************************************

std::string Message::readSocketBytes(Socket* socket, int numberBytes, bool& success) {
   if (numberBytes < MAX_STACK_BUFFER_SIZE) {
      char stackBuffer[MAX_STACK_BUFFER_SIZE];
      memset(stackBuffer, 0, MAX_STACK_BUFFER_SIZE);
      
      if (socket->readSocket(stackBuffer, numberBytes)) {
         stackBuffer[numberBytes] = '\0';
         success = true;
         return std::string(stackBuffer);
      } else {
         Logger::error("reading socket for header failed");
         success = false;
         return EMPTY_STRING;
      }
   } else {
      if (numberBytes <= MAX_SEGMENT_LENGTH) {
         std::string returnValue;
         char* heapBuffer = new char[numberBytes+1];
         memset(heapBuffer, 0, numberBytes+1);
         if (socket->readSocket(heapBuffer, numberBytes)) {
            heapBuffer[numberBytes] = '\0';
            success = true;
            returnValue = heapBuffer;
         } else {
            Logger::error("reading socket for header failed");
            success = false;
         }
         
         delete [] heapBuffer;
         
         return returnValue;
      } else {
         Logger::error("header length exceeds 32K");
         success = false;
         return EMPTY_STRING;
      }
   }   
}

//******************************************************************************
   
bool Message::reconstitute(Socket* socket) {
   if (socket != NULL) {
      
      char headerLengthPrefixBuffer[11];
      memset(headerLengthPrefixBuffer, 0, NUM_CHARS_HEADER_LENGTH+1);
      
      if (socket->readSocket(headerLengthPrefixBuffer, NUM_CHARS_HEADER_LENGTH)) {
         headerLengthPrefixBuffer[NUM_CHARS_HEADER_LENGTH] = '\0';
         
         std::string headerLengthPrefix = headerLengthPrefixBuffer;
         StrUtils::stripTrailing(headerLengthPrefix, ' ');
         const std::size_t headerLength = std::atol(headerLengthPrefix.c_str());
         
         if (headerLength > 0) {
            bool headerRead = false;
            std::string headerAsString =
               readSocketBytes(socket, headerLength, headerRead);
            
            if (headerRead && !headerAsString.empty()) {
               if (fromString(headerAsString, m_kvpHeaders)) {
                  if (m_kvpHeaders.hasKey(KEY_PAYLOAD_TYPE)) {
                     const std::string& valuePayloadType = m_kvpHeaders.getValue(KEY_PAYLOAD_TYPE);
                     
                     if (valuePayloadType == VALUE_PAYLOAD_TEXT) {
                        m_messageType = MessageType::Text;
                     } else if (valuePayloadType == VALUE_PAYLOAD_KVP) {
                        m_messageType = MessageType::KeyValues;
                     } else {
                        Logger::error("unrecognized payload type");
                     }
                  }
                  
                  if (m_messageType == MessageType::Unknown) {
                     Logger::error("unable to identify message type from header");
                     return false;
                  }
                  
                  if (m_kvpHeaders.hasKey(KEY_PAYLOAD_LENGTH)) {
                     const std::string& valuePayloadLength = m_kvpHeaders.getValue(KEY_PAYLOAD_LENGTH);
                     
                     if (!valuePayloadLength.empty()) {
                        const std::size_t payloadLength = std::atol(valuePayloadLength.c_str());
                        
                        if (payloadLength > 0) {
                           bool payloadRead = false;
                           std::string payloadAsString =
                              readSocketBytes(socket, payloadLength, payloadRead);
                           
                           if (payloadRead && !payloadAsString.empty()) {
                              if (m_messageType == MessageType::Text) {
                                 m_textPayload = payloadAsString;
                              } else if (m_messageType == MessageType::KeyValues) {
                                 fromString(payloadAsString, m_kvpPayload);
                              }
                           }
                        }
                     }
                  }
                  
                  if (m_kvpHeaders.hasKey(KEY_ONE_WAY)) {
                     const std::string& valueOneWay = m_kvpHeaders.getValue(KEY_ONE_WAY);
                     if (valueOneWay == VALUE_TRUE) {
                        // mark it as being a 1-way message
                        m_isOneWay = true;
                     }
                  }
                  
                  return true;
               } else {
                  // unable to parse header
                  Logger::error("unable to parse header");
               }
            } else {
               // unable to read header
               Logger::error("unable to read header");
            }
         } else {
            // header length is empty
            Logger::error("header length is empty");
         }
      } else {
         // socket read failed
         Logger::error("socket read failed");
      }
   } else {
      // no socket given
      Logger::error("no socket given to reconstitute");
   }

   return false;
}

//******************************************************************************

std::string Message::toString() const {
   KeyValuePairs kvpHeaders(m_kvpHeaders);
   std::string payload;
   
   if (m_messageType == MessageType::Text) {
      kvpHeaders.addPair(KEY_PAYLOAD_TYPE, VALUE_PAYLOAD_TEXT);
      payload = m_textPayload;
   } else if (m_messageType == MessageType::KeyValues) {
      kvpHeaders.addPair(KEY_PAYLOAD_TYPE, VALUE_PAYLOAD_KVP);
      payload = toString(m_kvpPayload);
   } else {
      kvpHeaders.addPair(KEY_PAYLOAD_TYPE, VALUE_PAYLOAD_UNKNOWN);
   }
   
   if (m_isOneWay) {
      kvpHeaders.addPair(KEY_ONE_WAY, VALUE_TRUE);
   }
   
   if (m_kvpHeaders.hasKey(KEY_REQUEST_NAME)) {
      kvpHeaders.addPair(KEY_REQUEST_NAME, m_kvpHeaders.getValue(KEY_REQUEST_NAME));
   } else {
      kvpHeaders.addPair(KEY_REQUEST_NAME, EMPTY_STRING);
   }

   const std::size_t payloadLength = payload.length();
   kvpHeaders.addPair(KEY_PAYLOAD_LENGTH, encodeLength(payloadLength));
   
   const std::string headersAsString = toString(kvpHeaders);
   
   std::string headerLengthPrefix = encodeLength(headersAsString.length());
   StrUtils::padRight(headerLengthPrefix, ' ', NUM_CHARS_HEADER_LENGTH);

   std::string messageAsString;
   messageAsString += headerLengthPrefix;
   messageAsString += headersAsString;
   messageAsString += payload;
   
   return messageAsString;
}

//******************************************************************************

std::string Message::toString(const KeyValuePairs& kvp) {
   std::string kvpAsString;
   
   if (!kvp.empty()) {
      std::vector<std::string> keys;
      kvp.getKeys(keys);
      int i = 0;
      
      for (auto key : keys) {
         if (i > 0) {
            // append pair delimiter
            kvpAsString += DELIMITER_PAIR;
         }
         
         kvpAsString += key;
         kvpAsString += DELIMITER_KEY_VALUE;
         kvpAsString += kvp.getValue(key);
         
         ++i;
      }
   }
      
   return kvpAsString;
}

//******************************************************************************

bool Message::fromString(const std::string& s, KeyValuePairs& kvp) {
   int numPairsAdded = 0;
   
   if (!s.empty()) {
      StringTokenizer stPairs(s, DELIMITER_PAIR);
      const std::size_t numPairs = stPairs.countTokens();
      
      if (numPairs > 0) {
         while (stPairs.hasMoreTokens()) {
            const std::string& keyValuePair = stPairs.nextToken();
            
            StringTokenizer stKeyValue(keyValuePair, DELIMITER_KEY_VALUE);
            const std::size_t numTokens = stKeyValue.countTokens();
            if (numTokens == 2) {
               const std::string& key = stKeyValue.nextToken();
               const std::string& value = stKeyValue.nextToken();
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
   char lengthAsChars[11];
   memset(lengthAsChars, 0, NUM_CHARS_HEADER_LENGTH+1);
   snprintf(lengthAsChars, NUM_CHARS_HEADER_LENGTH, "%lu", lengthBytes);
   return std::string(lengthAsChars);
}

//******************************************************************************

std::size_t Message::decodeLength(Socket* socket) {
   std::size_t lengthBytes = 0;
   
   if (socket != NULL) {
      char lengthAsChars[11];
      memset(lengthAsChars, 0, NUM_CHARS_HEADER_LENGTH+1);
      if (socket->read(lengthAsChars, NUM_CHARS_HEADER_LENGTH)) {
         return std::atol(lengthAsChars);
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

