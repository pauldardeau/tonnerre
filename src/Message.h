// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef MESSAGE_H
#define MESSAGE_H

#include <string>

#include "KeyValuePairs.h"

class Socket;


/**
 * The Message class is the primary object used for sending and receiving messages.
 * A new instance should be created for each message to send.  The messaging server
 * automatically creates the message instance (populated by the message handler
 * associated with the service) for the response.
 */
class Message
{
public:
   enum class MessageType {
      Unknown,
      KeyValues,
      Text
   };
   
   /**
    * Reconstructs a message by reading from a socket
    * @param socket the socket to read from
    * @return a new Message object instance constructed by reading data from socket
    * @see Socket()
    */
   static std::shared_ptr<Message> reconstruct(std::shared_ptr<Socket> socket);
   
   /**
    * Default constructor (used internally)
    */
   Message();
   
   /**
    * Constructs a message in anticipation of sending it
    * @param requestName the name of the message request
    * @param messageType the type of the message
    */
   Message(const std::string& requestName, MessageType messageType=MessageType::Unknown);
   
   /**
    * Copy constructor
    * @param copy the source of the copy construction
    */
   Message(const Message& copy);
   
   /**
    * Move constructor
    * @param move the source of the move construction
    */
   Message(Message&& move);
   
   /**
    * Destructor
    */
   ~Message();
   
   /**
    * Sends a message to the specified service and disregards any response that the
    * server handler might generate.
    * @param serviceName the name of the service destination
    * @return boolean indicating if message was successfully delivered
    */
   bool send(const std::string& serviceName);
   
   /**
    * Sends a message and retrieves the message response (synchronous call)
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
    * Move operator
    * @param move the source of the move
    * @return reference to the target of the move
    */
   Message& operator=(Message&& move);
   
   /**
    * Reconstitute a message by reading message state data from a socket (used internally)
    * @param socket the socket from which to read message state data
    * @return boolean indicating whether the message was successfully reconstituted
    * @see Socket()
    */
   bool reconstitute(std::shared_ptr<Socket> socket);
   
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
   const KeyValuePairs& getKeyValuesPayload() const;
   
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
   void setKeyValuesPayload(const KeyValuePairs& kvp);
   
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
   static std::string toString(const KeyValuePairs& kvp);
   
   /**
    * Reconstitutes the state of a KeyValuePairs from the specified string
    * @param s the textual data that holds the KeyValuePairs state data
    * @param kvp the KeyValuePairs object instance to populate
    * @return boolean indicating whether any state data was populated
    * @see KeyValuePairs()
    */
   static bool fromString(const std::string& s, KeyValuePairs& kvp);
   
   /**
    * Encodes a length to a string so that it can be encoded in flattened message (used internally)
    * @param the length in bytes to encode
    * @return the string representation of the length
    */
   static std::string encodeLength(std::size_t lengthBytes);
   
   /**
    * Decodes the length of the message header by reading from a socket (used internally)
    * @param socket the socket to read from
    * @return the decoded length of the message header
    * @see Socket()
    */
   static std::size_t decodeLength(std::shared_ptr<Socket> socket);
   
   /**
    * Retrieves a socket connection for the specified service (used internally)
    * @param serviceName the name of the service whose connection is needed
    * @param a Socket instance on success, nullptr on failure
    */
   std::shared_ptr<Socket> socketForService(const std::string& serviceName) const;
   
private:
   std::string m_serviceName;
   std::string m_textPayload;
   KeyValuePairs m_kvpPayload;
   KeyValuePairs m_kvpHeaders;
   MessageType m_messageType;
   bool m_isOneWay;
   
};

#endif
