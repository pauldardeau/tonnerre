// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef TONNERRE_MESSAGE_H
#define TONNERRE_MESSAGE_H

#include <string>

#include "KeyValuePairs.h"
#include "Socket.h"


namespace tonnerre
{

enum MessageType {
   MessageTypeUnknown,
   MessageTypeKeyValues,
   MessageTypeText
};


/**
 * The Message class is the primary object used for sending and receiving messages.
 * A new instance should be created for each message to send.  The messaging server
 * automatically creates the message instance (populated by the message handler
 * associated with the service) for the response.
 */
class Message
{
public:
   /**
    * Reconstructs a message by reading from a socket. On failure the socket
    * has been closed; see reconstitute().
    * @param socket the socket to read from
    * @return a new Message object instance constructed by reading data from socket,
    * or nullptr if a complete, valid message was not received
    * @see Socket()
    */
   static Message* reconstruct(chaudiere::Socket* socket);

   /**
    * Default constructor (used internally)
    */
   Message();

   /**
    * Constructs a message in anticipation of sending it
    * @param requestName the name of the message request
    * @param messageType the type of the message
    */
   Message(const std::string& requestName,
           MessageType messageType=MessageTypeUnknown);

   /**
    * Copy constructor
    * @param copy the source of the copy construction
    */
   Message(const Message& copy);

   /**
    * Destructor
    */
   ~Message();

   /**
    * Sends a message to the specified service and disregards any response that the
    * server handler might generate. A message whose header or payload exceeds
    * 32,767 bytes is not sent.
    * @param serviceName the name of the service destination
    * @return boolean indicating if message was successfully delivered
    */
   bool send(const std::string& serviceName);

   /**
    * Sends a message and retrieves the message response (synchronous call).
    * A message whose header or payload exceeds 32,767 bytes is not sent. If the
    * response is not received completely and validly, the connection is closed
    * and discarded rather than returned to the connection pool.
    * @param serviceName the name of the service destination
    * @param responseMessage the message object instance to populate with the response
    * @return boolean indicating if the message was successfully delivered and a response received
    */
   bool send(const std::string& serviceName, Message& responseMessage);

   /**
    * Copy operator
    * @param copy the source of the copy
    * @return reference to target of copy
    */
   Message& operator=(const Message& copy);

   /**
    * Reconstitute a message by reading message state data from a socket (used internally).
    *
    * The message is read with exact-length reads: the 10-character header-length
    * prefix, the header, and then the payload, each of which must arrive in full.
    * Header and payload are each limited to 32,767 bytes. The call returns false,
    * and never throws for malformed input, when the prefix or a length is not a
    * valid decimal number or exceeds its limit, when payload_type or payload_length
    * is missing or invalid, when a header pair is malformed, or when the peer
    * closes or the connection fails before the message is complete.
    *
    * On success the message's state is replaced by the received message. On
    * failure the message is left empty (type MessageTypeUnknown, no headers, no
    * payload) and the socket has been closed (Socket::close()): the unread
    * remainder of the stream cannot be trusted to begin at a message boundary, so
    * a connection on which reconstitute() has returned false must not be reused
    * or returned to a pool. Callers detect this with Socket::isConnected(), and
    * simply delete the socket.
    * @param socket the socket from which to read message state data
    * @return boolean indicating whether the message was successfully reconstituted
    * @see Socket()
    */
   bool reconstitute(chaudiere::Socket* socket);

   /**
    * Sets the type of the message
    * @param messageType the type of the message
    */
   void setType(MessageType messageType);

   /**
    * Retrieves the type of the message
    * @return the message type
    */
   MessageType getType() const;

   /**
    * Retrieves the name of the message request
    * @return the name of the message request
    */
   std::string getRequestName() const;

   /**
    * Retrieves the key/values payload associated with the message
    * @return reference to the key/values message payload
    * @see KeyValuePairs()
    */
   const chaudiere::KeyValuePairs& getKeyValuesPayload() const;

   /**
    * Retrieves the textual payload associated with the message
    * @return reference to the textual message payload
    */
   const std::string& getTextPayload() const;

   /**
    * Sets the key/values payload associated with the message
    * @param kvp the new key/values payload
    * @see KeyValuePairs()
    */
   void setKeyValuesPayload(const chaudiere::KeyValuePairs& kvp);

   /**
    * Sets the textual payload associated with the message
    * @param text the new textual payload
    */
   void setTextPayload(const std::string& text);

   /**
    * Retrieves the service name from a reconstituted message (used internally)
    * @return the name of the service
    */
   const std::string& getServiceName() const;

   /**
    * Flatten the message state to a string so that it can be sent over network connection (used internally)
    * @return string representation of message state ready to be sent over network
    */
   std::string toString() const;

   /**
    * Flatten a KeyValuePairs object as part of flattening the Message
    * @param kvp the KeyValuePairs object whose string representation is needed
    * @return the string representation of the KeyValuePairs object
    */
   static std::string toString(const chaudiere::KeyValuePairs& kvp);

   /**
    * Reconstitutes the state of a KeyValuePairs from the specified string
    * @param s the textual data that holds the KeyValuePairs state data
    * @param kvp the KeyValuePairs object instance to populate
    * @return boolean indicating whether any state data was populated
    * @see KeyValuePairs()
    */
   static bool fromString(const std::string& s, chaudiere::KeyValuePairs& kvp);

   /**
    * Encodes a length to a string so that it can be encoded in flattened message (used internally)
    * @param lengthBytes the length in bytes to encode
    * @return the string representation of the length
    */
   static std::string encodeLength(std::size_t lengthBytes);

   /**
    * Decodes the length of the message header by reading from a socket (used internally)
    * @param socket the socket to read from
    * @return the decoded length of the message header
    * @see Socket()
    */
   static std::size_t decodeLength(chaudiere::Socket* socket);

   /**
    * Retrieves a socket connection for the specified service (used internally)
    * @param serviceName the name of the service whose connection is needed
    * @return a Socket instance on success, nullptr on failure
    */
   chaudiere::Socket* socketForService(const std::string& serviceName) const;

   /**
    *
    * @param serviceName
    * @param socket
    */
   void returnSocketForService(const std::string& serviceName,
                               chaudiere::Socket* socket);

   /**
    * Sets the specified key/value pair in the headers
    * @param key the new header key
    * @param value the new header value
    */
   void setHeader(const std::string& key, const std::string& value);

   /**
    * Determines if the specified key exists in the headers
    * @param key whose existence is being tested
    * @return boolean indicating whether the key exists in the headers
    */
   bool hasHeader(const std::string& key) const;

   /**
    * Retrieves the header value associated with the specified key
    * @param key header key whose value is being retrieved
    * @return reference to header value
    * @throw InvalidKeyException
    */
   const std::string& getHeader(const std::string& key) const;

   /**
    * Reads exactly numberBytes bytes from the socket (used internally)
    * @param socket the socket to read from
    * @param numberBytes the number of bytes to read, 0 to 32,767
    * @param success set true only if all numberBytes bytes were received; false
    * for a negative or oversized count, a short read, end-of-stream or an error
    * @return the bytes read (empty if success is false)
    */
   std::string readSocketBytes(chaudiere::Socket* socket,
                               int numberBytes,
                               bool& success);

private:
   // MessageRequestHandler must not send a response the client would reject
   friend class MessageRequestHandler;

   // flattens the message like toString(); returns false if the header or the
   // payload exceeds the size limit that receivers enforce
   bool toWireString(std::string& messageAsString) const;

   // returns the message to its empty state
   void clearReceivedState();

   std::string m_serviceName;
   std::string m_textPayload;
   chaudiere::KeyValuePairs m_kvpPayload;
   chaudiere::KeyValuePairs m_kvpHeaders;
   MessageType m_messageType;
   bool m_isOneWay;
   mutable bool m_persistentConnection;

};

}

#endif
