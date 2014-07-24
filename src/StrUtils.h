// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef StrUtils_h
#define StrUtils_h

#include <string>

/*!
 * StrUtils is a utility class of convenience methods for working with strings.
 */
class StrUtils
{
public:
   static std::string strip(const std::string& s) noexcept;
   static std::string strip(const std::string& s, char strip) noexcept;
    
   static bool startsWith(const std::string& haystack,
                          const std::string& needle) noexcept;
   static bool endsWith(const std::string& haystack,
                        const std::string& needle) noexcept;
   static bool containsString(const std::string& haystack,
                              const std::string& needle) noexcept;
    
   static void toUpperCase(std::string& s) noexcept;
   static void toLowerCase(std::string& s) noexcept;
    
   static std::string& replaceAll(std::string& s,
                                  const std::string& searchFor,
                                  const std::string& replaceWith) noexcept;
    
   static std::string& strip(std::string& s, char strip=' ') noexcept;
   static std::string& stripTrailing(std::string& s, char strip) noexcept;
   static std::string& stripLeading(std::string& s, char strip) noexcept;
   static std::string& trimLeadingSpaces(std::string& s) noexcept;
   
   static std::string gzipCompress(const std::string& s);
   static std::string gzipDecompress(const std::string& s);
   
};


#endif
