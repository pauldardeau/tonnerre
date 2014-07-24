#include "Message.h"

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

Message::Message(const MessageType& messageType) :
   m_messageType(messageType)
{
   
}

//******************************************************************************

Message::Message(const Message& copy) :
   m_textPayload(copy.m_textPayload),
   m_kvpPayload(copy.m_kvpPayload),
   m_messageType(copy.m_messageType)
{
}

//******************************************************************************

Message::Message(Message&& move) :
   m_textPayload(std::move(move.m_textPayload)),
   m_kvpPayload(std::move(move.m_kvpPayload)),
   m_messageType(move.m_messageType)
{
   
}

//******************************************************************************

Message::~Message()
{
   
}

//******************************************************************************

Message& Message::operator=(const Message& copy)
{
   if (this == &copy) {
      return *this;
   }
   
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
   
   m_textPayload = std::move(move.m_textPayload);
   m_kvpPayload = std::move(move.m_kvpPayload);
   m_messageType = move.m_messageType;
   
   return *this;
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

bool Message::reconstitute(std::shared_ptr<Socket> socket)
{
   return false;
}

//******************************************************************************
