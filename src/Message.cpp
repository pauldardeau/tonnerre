// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#include <string>
#include <vector>
#include <cstdlib>

#include "Message.h"
#include "Logger.h"
#include "StrUtils.h"
#include "Socket.h"
#include "StringTokenizer.h"

static const int MAX_STACK_BUFFER_SIZE          = 4096;
static const int MAX_SEGMENT_LENGTH             = 32767;

static const int NUM_CHARS_HEADER_LENGTH        = 10;

static const std::string DELIMITER_KEY_VALUE    = "=";
static const std::string DELIMITER_PAIR         = ";";

static const std::string KEY_ONE_WAY            = "1way";
static const std::string KEY_PAYLOAD_LENGTH     = "payload_length";
static const std::string KEY_PAYLOAD_TYPE       = "payload_type";

static const std::string VALUE_PAYLOAD_KVP      = "kvp";
static const std::string VALUE_PAYLOAD_TEXT     = "text";
static const std::string VALUE_PAYLOAD_UNKNOWN  = "unknown";
static const std::string VALUE_TRUE             = "true";


//******************************************************************************

std::shared_ptr<Message> Message::reconstruct(std::shared_ptr<Socket> socket)
{
   std::shared_ptr<Message> message(new Message(Message::MessageType::Unknown));
   if (message->reconstitute(socket)) {
      return message;
   } else {
      return nullptr;
   }
}

//******************************************************************************

Message::Message(MessageType messageType) :
   m_messageType(messageType),
   m_isOneWay(false)
{
   Logger::logInstanceCreate("Message");
}

//******************************************************************************

Message::Message(const Message& copy) :
   m_serviceName(copy.m_serviceName),
   m_textPayload(copy.m_textPayload),
   m_kvpPayload(copy.m_kvpPayload),
   m_kvpHeaders(copy.m_kvpHeaders),
   m_messageType(copy.m_messageType),
   m_isOneWay(copy.m_isOneWay)
{
   Logger::logInstanceCreate("Message");
}

//******************************************************************************

Message::Message(Message&& move) :
   m_serviceName(std::move(move.m_serviceName)),
   m_textPayload(std::move(move.m_textPayload)),
   m_kvpPayload(std::move(move.m_kvpPayload)),
   m_kvpHeaders(std::move(move.m_kvpHeaders)),
   m_messageType(move.m_messageType),
   m_isOneWay(move.m_isOneWay)
{
   Logger::logInstanceCreate("Message");
}

//******************************************************************************

Message::~Message()
{
   Logger::logInstanceDestroy("Message");
}

//******************************************************************************

Message& Message::operator=(const Message& copy)
{
   if (this == &copy) {
      return *this;
   }
   
   m_serviceName = copy.m_serviceName;
   m_textPayload = copy.m_textPayload;
   m_kvpPayload = copy.m_kvpPayload;
   m_kvpHeaders = copy.m_kvpHeaders;
   m_messageType = copy.m_messageType;
   m_isOneWay = copy.m_isOneWay;
   
   return *this;
}

//******************************************************************************

Message& Message::operator=(Message&& move)
{
   if (this == &move) {
      return *this;
   }
   
   m_serviceName = std::move(move.m_serviceName);
   m_textPayload = std::move(move.m_textPayload);
   m_kvpPayload = std::move(move.m_kvpPayload);
   m_kvpHeaders = std::move(move.m_kvpHeaders);
   m_messageType = move.m_messageType;
   m_isOneWay = move.m_isOneWay;
   
   return *this;
}

//******************************************************************************

bool Message::send(const std::string& serviceName)
{
   if (m_messageType == MessageType::Unknown) {
      Logger::error("unable to send message, no message type set");
      return false;
   }

   std::shared_ptr<Socket> socket(socketForService(serviceName));
   
   if (socket != nullptr) {
      m_isOneWay = true;
      
      if (socket->write(toString())) {
         return true;
      } else {
         // unable to write to socket
         Logger::error("unable to write to socket");
      }
   } else {
      // unable to connect to service
      Logger::error("unable to connect to service");
   }
   
   return false;
}

//******************************************************************************

bool Message::send(const std::string& serviceName, Message& responseMessage)
{
   if (m_messageType == MessageType::Unknown) {
      Logger::error("unable to send message, no message type set");
      return false;
   }
   
   std::shared_ptr<Socket> socket(socketForService(serviceName));
   
   if (socket != nullptr) {
      if (socket->write(toString())) {
         return responseMessage.reconstitute(socket);
      } else {
         // unable to write to socket
         Logger::error("unable to write to socket");
      }
   } else {
      // unable to connect to service
      Logger::error("unable to connect to service");
   }

   return false;
}

//******************************************************************************

Message::MessageType Message::getType() const
{
   return m_messageType;
}

//******************************************************************************

const KeyValuePairs& Message::getKeyValuesPayload() const
{
   return m_kvpPayload;
}

//******************************************************************************

const std::string& Message::getTextPayload() const
{
   return m_textPayload;
}

//******************************************************************************

void Message::setKeyValuesPayload(const KeyValuePairs& kvp)
{
   m_kvpPayload = kvp;
}

//******************************************************************************

void Message::setTextPayload(const std::string& text)
{
   m_textPayload = text;
}

//******************************************************************************

const std::string& Message::getServiceName() const
{
   return m_serviceName;
}

//******************************************************************************

std::shared_ptr<Socket> Message::socketForService(const std::string& serviceName) const
{
   //TODO: implement socketForService
   return nullptr;
}

//******************************************************************************

bool Message::reconstitute(std::shared_ptr<Socket> socket)
{
   if (socket != nullptr) {
      char headerLengthPrefixBuffer[11];
      memset(headerLengthPrefixBuffer, 0, NUM_CHARS_HEADER_LENGTH+1);
      
      if (socket->read(headerLengthPrefixBuffer, NUM_CHARS_HEADER_LENGTH)) {
         headerLengthPrefixBuffer[NUM_CHARS_HEADER_LENGTH] = '\0';
         
         std::string headerLengthPrefix = headerLengthPrefixBuffer;
         StrUtils::stripTrailing(headerLengthPrefix, ' ');
         const std::size_t headerLength = std::atol(headerLengthPrefix.c_str());
         
         if (headerLength > 0) {
            char stackBuffer[MAX_STACK_BUFFER_SIZE];
            bool stackBufferZeroed = false;
            bool headerRead = false;
            std::string headerAsString;
            
            if (headerLength < MAX_STACK_BUFFER_SIZE) {
               memset(stackBuffer, 0, MAX_STACK_BUFFER_SIZE);
               stackBufferZeroed = true;
               
               if (socket->read(stackBuffer, headerLength)) {
                  stackBuffer[headerLength] = '\0';
                  headerRead = true;
                  headerAsString = stackBuffer;
               } else {
                  Logger::error("reading socket for header failed");
                  return false;
               }
            } else {
               if (headerLength <= MAX_SEGMENT_LENGTH) {
                  char* heapBuffer = new char[headerLength+1];
                  memset(heapBuffer, 0, headerLength+1);
                  if (socket->read(heapBuffer, headerLength)) {
                     heapBuffer[headerLength] = '\0';
                     headerRead = true;
                     headerAsString = heapBuffer;
                  } else {
                     delete [] heapBuffer;
                     Logger::error("reading socket for header failed");
                     return false;
                  }
                  delete [] heapBuffer;
               } else {
                  Logger::error("header length exceeds 32K");
                  return false;
               }
            }
            
            if (headerRead && !headerAsString.empty()) {
               KeyValuePairs kvpHeaders;
               if (fromString(headerAsString, kvpHeaders)) {
                  if (kvpHeaders.hasKey(KEY_PAYLOAD_TYPE)) {
                     const std::string& valuePayloadType = kvpHeaders.getValue(KEY_PAYLOAD_TYPE);
                     
                     if (valuePayloadType == VALUE_PAYLOAD_TEXT) {
                        m_messageType = MessageType::Text;
                     } else if (valuePayloadType == VALUE_PAYLOAD_KVP) {
                        m_messageType = MessageType::KeyValues;
                     }
                  }
                  
                  if (m_messageType == MessageType::Unknown) {
                     Logger::error("unable to identify message type from header");
                     return false;
                  }
                  
                  if (kvpHeaders.hasKey(KEY_PAYLOAD_LENGTH)) {
                     const std::string& valuePayloadLength = kvpHeaders.getValue(KEY_PAYLOAD_LENGTH);
                     
                     if (!valuePayloadLength.empty()) {
                        const std::size_t payloadLength = std::atol(valuePayloadLength.c_str());
                        
                        if (payloadLength > 0) {
                           bool payloadRead = false;
                           std::string payloadAsString;
                           
                           if (payloadLength < MAX_STACK_BUFFER_SIZE) {
                              memset(stackBuffer, 0, MAX_STACK_BUFFER_SIZE);
                              
                              if (socket->read(stackBuffer, payloadLength)) {
                                 stackBuffer[payloadLength] = '\0';
                                 payloadRead = true;
                                 payloadAsString = stackBuffer;
                              } else {
                                 Logger::error("reading socket for payload failed");
                                 return false;
                              }
                           } else {
                              if (payloadLength <= MAX_SEGMENT_LENGTH) {
                                 char* heapBuffer = new char[payloadLength];
                                 memset(heapBuffer, 0, payloadLength);
                                 if (socket->read(heapBuffer, payloadLength)) {
                                    heapBuffer[payloadLength] = '\0';
                                    payloadRead = true;
                                    payloadAsString = heapBuffer;
                                 } else {
                                    delete [] heapBuffer;
                                    Logger::error("reading socket for payload failed");
                                    return false;
                                 }
                                 delete [] heapBuffer;
                              } else {
                                 Logger::error("payload length exceeds 32K");
                                 return false;
                              }
                           }
                           
                           if (payloadRead && !payloadAsString.empty()) {
                              if (m_messageType == MessageType::Text) {
                                 m_textPayload = std::move(payloadAsString);
                              } else if (m_messageType == MessageType::KeyValues) {
                                 KeyValuePairs kvpPayload;
                                 if (fromString(payloadAsString, kvpPayload)) {
                                    m_kvpPayload = std::move(kvpPayload);
                                 }
                              }
                           }
                        }
                     }
                  }
                  
                  if (kvpHeaders.hasKey(KEY_ONE_WAY)) {
                     const std::string& valueOneWay = kvpHeaders.getValue(KEY_ONE_WAY);
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

std::string Message::toString() const
{
   std::size_t payloadLength = 0;
   
   KeyValuePairs kvpHeaders(m_kvpHeaders);
   
   if (m_messageType == MessageType::Text) {
      kvpHeaders.addPair(KEY_PAYLOAD_TYPE, VALUE_PAYLOAD_TEXT);
      payloadLength = m_textPayload.length();
   } else if (m_messageType == MessageType::KeyValues) {
      kvpHeaders.addPair(KEY_PAYLOAD_TYPE, VALUE_PAYLOAD_KVP);
   } else {
      kvpHeaders.addPair(KEY_PAYLOAD_TYPE, VALUE_PAYLOAD_UNKNOWN);
   }
   
   if (m_isOneWay) {
      kvpHeaders.addPair(KEY_ONE_WAY, VALUE_TRUE);
   }
   
   kvpHeaders.addPair(KEY_PAYLOAD_LENGTH, encodeLength(payloadLength));
   
   const std::string headersAsString = toString(kvpHeaders);

   std::string headerLengthPrefix = encodeLength(headersAsString.length());
   StrUtils::padRight(headerLengthPrefix, ' ', NUM_CHARS_HEADER_LENGTH);

   std::string messageAsString;
   messageAsString += headerLengthPrefix;
   messageAsString += headersAsString;
   
   if (m_messageType == MessageType::Text) {
      messageAsString += m_textPayload;
   } else if (m_messageType == MessageType::KeyValues) {
      messageAsString += toString(m_kvpPayload);
   }
   
   return messageAsString;
}

//******************************************************************************

std::string Message::toString(const KeyValuePairs& kvp)
{
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

bool Message::fromString(const std::string& s, KeyValuePairs& kvp)
{
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

std::string Message::encodeLength(std::size_t lengthBytes)
{
   char lengthAsChars[11];
   memset(lengthAsChars, 0, NUM_CHARS_HEADER_LENGTH+1);
   snprintf(lengthAsChars, NUM_CHARS_HEADER_LENGTH, "%lu", lengthBytes);
   return std::string(lengthAsChars);
}

//******************************************************************************

std::size_t Message::decodeLength(std::shared_ptr<Socket> socket)
{
   std::size_t lengthBytes = 0;
   
   if (socket != nullptr) {
      char lengthAsChars[11];
      memset(lengthAsChars, 0, NUM_CHARS_HEADER_LENGTH+1);
      if (socket->read(lengthAsChars, NUM_CHARS_HEADER_LENGTH)) {
         return std::atol(lengthAsChars);
      }
   }
   
   return lengthBytes;
}

//******************************************************************************

