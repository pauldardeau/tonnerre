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
   
   Message(MessageType messageType=MessageType::Unknown);
   Message(const Message& copy);
   Message(Message&& move);
   ~Message();
   
   bool send(const std::string& serviceName);
   bool send(const std::string& serviceName, Message& responseMessage);
   
   Message& operator=(const Message& copy);
   Message& operator=(Message&& move);
   
   bool reconstitute(std::shared_ptr<Socket> socket);
   
   void setType(MessageType messageType);
   MessageType getType() const;
   const std::string getRequestName() const;
   
   const KeyValuePairs& getKeyValuesPayload() const;
   const std::string& getTextPayload() const;
   
   void setKeyValuesPayload(const KeyValuePairs& kvp);
   void setTextPayload(const std::string& text);

   const std::string& getServiceName() const;
   
private:
   std::string m_serviceName;
   std::string m_textPayload;
   KeyValuePairs m_kvpPayload;
   MessageType m_messageType;
   
};

#endif
