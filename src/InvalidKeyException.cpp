// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#include "InvalidKeyException.h"
#include "Logger.h"

//******************************************************************************

InvalidKeyException::InvalidKeyException(const std::string& key) noexcept :
   BasicException("Invalid Key: " +  key),
   m_key(key)
{
   Logger::logInstanceCreate("InvalidKeyException");
}

//******************************************************************************

InvalidKeyException::InvalidKeyException(const InvalidKeyException& copy) noexcept :
   BasicException(copy),
   m_key(copy.m_key)
{
   Logger::logInstanceCreate("InvalidKeyException");
}

//******************************************************************************

InvalidKeyException::InvalidKeyException(InvalidKeyException&& move) noexcept :
   BasicException(move),
   m_key(std::move(move.m_key))
{
   Logger::logInstanceCreate("InvalidKeyException");
}

//******************************************************************************

InvalidKeyException::~InvalidKeyException() noexcept
{
   Logger::logInstanceDestroy("InvalidKeyException");
}

//******************************************************************************

InvalidKeyException& InvalidKeyException::operator=(const InvalidKeyException& copy) noexcept
{
   if (this == &copy) {
      return *this;
   }
   
   BasicException::operator=(copy);
   m_key = copy.m_key;
   
   return *this;
}

//******************************************************************************

InvalidKeyException& InvalidKeyException::operator=(InvalidKeyException&& move) noexcept
{
   if (this == &move) {
      return *this;
   }
   
   BasicException::operator=(move);
   m_key = std::move(move.m_key);
   
   return *this;
}

//******************************************************************************

const std::string& InvalidKeyException::getKey() const noexcept
{
   return m_key;
}

//******************************************************************************

