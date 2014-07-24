#include "Message.h"
#include "Logger.h"

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
   m_messageType(messageType)
{
   Logger::logInstanceCreate("Message");
}

//******************************************************************************

Message::Message(const Message& copy) :
   m_serviceName(copy.m_serviceName),
   m_textPayload(copy.m_textPayload),
   m_kvpPayload(copy.m_kvpPayload),
   m_messageType(copy.m_messageType)
{
   Logger::logInstanceCreate("Message");
}

//******************************************************************************

Message::Message(Message&& move) :
   m_serviceName(std::move(move.m_serviceName)),
   m_textPayload(std::move(move.m_textPayload)),
   m_kvpPayload(std::move(move.m_kvpPayload)),
   m_messageType(move.m_messageType)
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
   m_messageType = copy.m_messageType;
   
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
   m_messageType = move.m_messageType;
   
   return *this;
}

//******************************************************************************

bool Message::send(const std::string& serviceName)
{
   return false;
}

//******************************************************************************

bool Message::send(const std::string& serviceName, Message& responseMessage)
{
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

bool Message::reconstitute(std::shared_ptr<Socket> socket)
{
   return false;
}

//******************************************************************************
