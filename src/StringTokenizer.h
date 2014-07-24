// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef STRINGTOKENIZER_H
#define STRINGTOKENIZER_H

#include <string>
#include <vector>

/*!
 * StringTokenizer is very similar to Java's StringTokenizer.
 */
class StringTokenizer
{
   public:
      /**
       * Constructs a StringTokenizer and tokenizes the specified string
       * @param s the string to tokenize
       */
      explicit StringTokenizer(const std::string& s) noexcept;
   
      /**
       * Constructs a StringTokenizer with the string to tokenize and the delimiter
       * @param s the string to tokenize
       * @param delimiter the delimiter of the tokens
       */
      StringTokenizer(const std::string& s,
                      const std::string& delimiter) noexcept;
   
      /**
       * Destructor
       */
      ~StringTokenizer() noexcept;

      /**
       * Determines whether more tokens are present
       * @return boolean indicating if there are more tokens available
       */
      bool hasMoreTokens() const noexcept;
   
      /**
       * Retrieves the next available token
       * @throw std::out_of_range
       * @return the next token
       */
      const std::string& nextToken();
   
      /**
       * Retrieves the number of tokens found
       * @return the number of tokens
       */
      std::size_t countTokens() const noexcept;
   
      //disallow copies
      StringTokenizer(const StringTokenizer&) = delete;
      StringTokenizer(StringTokenizer&&) = delete;
      StringTokenizer& operator=(const StringTokenizer&) = delete;
      StringTokenizer& operator=(StringTokenizer&&) = delete;

   
   protected:
      /**
       * Extracts the next available token as part of the construction (tokenizing)
       * @throw std::out_of_range
       * @return the next available token
       */
      std::string extractNextToken();

   
   private:
   
      const std::string& m_withTokens;
      const std::string m_delimiter;
      const char* m_posTokens;
      const char* m_posDelimiter;
      std::string::size_type m_posCurrent;
      std::string::size_type m_stringLength;
      bool m_isConstructing;
      std::vector<std::string> m_tokens;
      std::size_t m_numberTokens;
      std::size_t m_indexToken;
};

#endif


