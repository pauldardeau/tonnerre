// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#include "KeyValuePairs.h"
#include "InvalidKeyException.h"
#include "Logger.h"

//******************************************************************************

KeyValuePairs::KeyValuePairs() noexcept
{
   Logger::logInstanceCreate("KeyValuePairs");
}

//******************************************************************************

KeyValuePairs::KeyValuePairs(const KeyValuePairs& copy) noexcept :
   m_keyValues(copy.m_keyValues)
{
   Logger::logInstanceCreate("KeyValuePairs");
}

//******************************************************************************

KeyValuePairs::KeyValuePairs(KeyValuePairs&& move) noexcept :
   m_keyValues(std::move(move.m_keyValues))
{
   Logger::logInstanceCreate("KeyValuePairs");
}

//******************************************************************************

KeyValuePairs::~KeyValuePairs() noexcept
{
   Logger::logInstanceDestroy("KeyValuePairs");
}

//******************************************************************************

KeyValuePairs& KeyValuePairs::operator=(const KeyValuePairs& copy) noexcept
{
   if (&copy == this) {
      return *this;
   }
   
   m_keyValues = copy.m_keyValues;

   return *this;
}

//******************************************************************************

KeyValuePairs& KeyValuePairs::operator=(KeyValuePairs&& move) noexcept
{
   if (&move == this) {
      return *this;
   }
   
   m_keyValues = std::move(move.m_keyValues);

   return *this;
}

//******************************************************************************

void KeyValuePairs::getKeys(std::vector<std::string>& keys) const noexcept
{
   if (m_keyValues.empty()) {
      return;
   }

   keys.reserve(m_keyValues.size());
   
   for (auto kv : m_keyValues) {
      keys.push_back(kv.first);
   }
}

//******************************************************************************

bool KeyValuePairs::hasKey(const std::string& key) const noexcept
{
   return (m_keyValues.find(key) != m_keyValues.end());
}

//******************************************************************************

const std::string& KeyValuePairs::getValue(const std::string& key) const
{
   auto it = m_keyValues.find(key);
   if (it != m_keyValues.end()) {
      return (*it).second;
   } else {
      throw InvalidKeyException(key);
      return (*it).second;
   }
}

//******************************************************************************

void KeyValuePairs::addPair(const std::string& key, const std::string& value) noexcept
{
   m_keyValues[key] = value;
}

//******************************************************************************

bool KeyValuePairs::removePair(const std::string& key) noexcept
{
   auto it = m_keyValues.find(key);
   if (it != m_keyValues.end()) {
      m_keyValues.erase(it);
      return true;
   }
   
   return false;
}

//******************************************************************************

void KeyValuePairs::clear() noexcept
{
   m_keyValues.erase(m_keyValues.begin(), m_keyValues.end());
}

//******************************************************************************

std::size_t KeyValuePairs::size() const noexcept
{
   return m_keyValues.size();
}

//******************************************************************************

bool KeyValuePairs::empty() const noexcept
{
   return m_keyValues.empty();
}

//******************************************************************************

