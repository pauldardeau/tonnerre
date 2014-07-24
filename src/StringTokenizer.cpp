// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#include <cstdio>
#include <cstring>

#include "StringTokenizer.h"
#include "BasicException.h"
#include "Logger.h"

static const std::string SPACE = " ";


//******************************************************************************

StringTokenizer::StringTokenizer(const std::string& withTokens) noexcept :
   StringTokenizer(withTokens, SPACE)
{
}

//******************************************************************************

StringTokenizer::StringTokenizer(const std::string& withTokens,
                                 const std::string& delimiter) noexcept :
   m_withTokens(withTokens),
   m_delimiter(delimiter),
   m_posTokens(m_withTokens.c_str()),
   m_posDelimiter(m_delimiter.c_str()),
   m_posCurrent(0),
   m_stringLength(m_withTokens.length()),
   m_isConstructing(true),
   m_numberTokens(0),
   m_indexToken(0)
{
   Logger::logInstanceCreate("StringTokenizer");

   if (m_withTokens.empty()) {
      m_posCurrent = std::string::npos;
   } else {
      m_posCurrent = ::strspn(m_posTokens, m_posDelimiter);
         
      while (m_posCurrent != std::string::npos) { // while (hasMoreTokens()) {
         m_tokens.push_back(extractNextToken());
      }
         
      m_numberTokens = m_tokens.size();
   }
      
   m_isConstructing = false;
}

//******************************************************************************

StringTokenizer::~StringTokenizer() noexcept
{
   Logger::logInstanceDestroy("StringTokenizer");
}

//******************************************************************************

bool StringTokenizer::hasMoreTokens() const noexcept
{
   if (m_isConstructing) {
      return (m_posCurrent != std::string::npos);
   } else {
      return (m_indexToken < m_numberTokens);
   }
}

//******************************************************************************

std::size_t StringTokenizer::countTokens() const noexcept
{
   return m_numberTokens;
}

//******************************************************************************

const std::string& StringTokenizer::nextToken()
{
   if (m_indexToken < m_numberTokens) {
      return m_tokens[m_indexToken++];
   } else {
      throw std::out_of_range("no more tokens");
   }
}

//******************************************************************************

std::string StringTokenizer::extractNextToken()
{
   if (m_posCurrent == std::string::npos) {
      throw std::out_of_range("no more tokens");
   }
   
   const char* posWithTokens = m_posTokens + m_posCurrent;
   
   const size_t numNonDelimiterChars = ::strcspn(posWithTokens, m_posDelimiter);
   
   if (numNonDelimiterChars > 0) {
      auto posStart = m_posCurrent;
      m_posCurrent += numNonDelimiterChars;
      posWithTokens += numNonDelimiterChars;
      
      m_posCurrent += ::strspn(posWithTokens, m_posDelimiter);
      
      if (m_posCurrent >= m_stringLength) {
         m_posCurrent = std::string::npos;
      }
      
      return m_withTokens.substr(posStart, numNonDelimiterChars);
   } else {
      // we're on the last token
      const std::string::size_type posLastToken = m_posCurrent;
      m_posCurrent = std::string::npos;
      
      if (m_posCurrent >= m_stringLength) {
         m_posCurrent = std::string::npos;
      }
      
      return m_withTokens.substr(posLastToken);
   }
}

//******************************************************************************
