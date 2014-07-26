// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef STRUTILS_H
#define STRUTILS_H

#include <string>

/**
 * StrUtils is a utility class of convenience methods for working with strings.
 */
class StrUtils
{
public:
   /**
    *
    * @param s
    * @return
    */
   static std::string strip(const std::string& s) noexcept;
   
  /**
   *
   * @param s
   * @param strip
   * @return
   */
   static std::string strip(const std::string& s, char strip) noexcept;
    
   /**
    *
    * @param haystack
    * @param needle
    * @return
    */
   static bool startsWith(const std::string& haystack,
                          const std::string& needle) noexcept;
                          
   /**
    *
    * @param haystack
    * @param needle
    * @return
    */
   static bool endsWith(const std::string& haystack,
                        const std::string& needle) noexcept;

   /**
    *
    * @param haystack
    * @param needle
    * @return
    */
   static bool containsString(const std::string& haystack,
                              const std::string& needle) noexcept;

   /**
    *
    * @param s
    */
   static void toUpperCase(std::string& s) noexcept;
   
   /**
    *
    * @param s
    */
   static void toLowerCase(std::string& s) noexcept;

   /**
    *
    * @param s
    * @param searchFor
    * @param replaceWith
    * @return
    */
   static std::string& replaceAll(std::string& s,
                                  const std::string& searchFor,
                                  const std::string& replaceWith) noexcept;

   /**
    *
    * @param s
    * @param strip
    * @return
    */
   static std::string& strip(std::string& s, char strip=' ') noexcept;
   
   /**
    *
    * @param s
    * @param strip
    */
   static std::string& stripTrailing(std::string& s, char strip) noexcept;
   
   /**
    *
    * @param s
    * @param strip
    * @return
    */
   static std::string& stripLeading(std::string& s, char strip) noexcept;
   
   /**
    *
    * @param s
    * @return
    */
   static std::string& trimLeadingSpaces(std::string& s) noexcept;
   
   /**
    *
    * @param s
    * @param padChar
    * @param paddedLength
    */
   static void padRight(std::string& s, char padChar, int paddedLength);

   //static std::string gzipCompress(const std::string& s);
   //static std::string gzipDecompress(const std::string& s);
   
};


#endif
