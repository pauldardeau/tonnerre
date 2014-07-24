#ifndef MESSAGE_H
#define MESSAGE_H

#include <string>

#include "KeyValuePairs.h"

class Socket;


class Message
{
public:
   enum class MessageType {
      Unknown,
      KeyValues,
      Text
   };
   
   static std::shared_ptr<Message> reconstruct(std::shared_ptr<Socket> socket);
   
   explicit Message(const MessageType& messageType);
   Message(const Message& copy);
   Message(Message&& move);
   ~Message();
   
   Message& operator=(const Message& copy);
   Message& operator=(Message&& move);
   
   bool reconstitute(std::shared_ptr<Socket> socket);
   
   MessageType getType() const;
   const std::string getRequestName() const;
   
   const KeyValuePairs& getKeyValuesPayload() const;
   const std::string& getTextPayload() const;
   
   void setKeyValuesPayload(const KeyValuePairs& kvp);
   void setTextPayload(const std::string& text);

private:
   std::string m_textPayload;
   KeyValuePairs m_kvpPayload;
   MessageType m_messageType;
   
};

#endif
