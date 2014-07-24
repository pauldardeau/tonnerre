// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#include <sstream>
#include <stdexcept>
#include <string.h>

#include "StrUtils.h"
#include "zlib.h"

static const std::string EMPTY = "";
static const std::string SPACE = " ";

//******************************************************************************

void StrUtils::toLowerCase(std::string& s) noexcept
{
   if (!s.empty()) {
      for (auto &c : s) {
         c = tolower(c);
      }
   }
}

//******************************************************************************

void StrUtils::toUpperCase(std::string& s) noexcept
{
   if (!s.empty()) {
      for (auto &c : s) {
         c = toupper(c);
      }
   }
}

//******************************************************************************

bool StrUtils::startsWith(const std::string& haystack, const std::string& needle) noexcept
{
   if (!haystack.empty() && !needle.empty()) {
      const std::string::size_type haystackLength = haystack.length();
      const std::string::size_type needleLength = needle.length();
        
      if (haystackLength >= needleLength) {
         for (std::string::size_type i = 0; i < needleLength; ++i) {
            if (haystack[i] != needle[i]) {
               return false;
            }
         }
            
         return true;
      }
   }
   
   return false;
}

//******************************************************************************

bool StrUtils::endsWith(const std::string& haystack, const std::string& needle) noexcept
{
   if (!haystack.empty() && !needle.empty()) {
      const std::string::size_type haystackLength = haystack.length();
      const std::string::size_type needleLength = needle.length();
        
      if (haystackLength >= needleLength) {
         unsigned long int haystackIndex = haystackLength - 1;
         
         for (long i = needleLength-1; i >= 0; --i) {
            if (haystack[haystackIndex--] != needle[i]) {
               return false;
            }
         }
            
         return true;
      }
   }

   return false;
}

//******************************************************************************

bool StrUtils::containsString(const std::string& haystack, const std::string& needle) noexcept
{
   return (std::string::npos != haystack.find(needle));
}

//******************************************************************************

std::string& StrUtils::strip(std::string& s, char strip) noexcept
{
   return stripLeading(stripTrailing(s, strip), strip);
}

//******************************************************************************

std::string& StrUtils::stripTrailing(std::string& s, char strip) noexcept
{
   if (s.empty()) {
      return s;
   }
   
   const std::string::size_type stringLen = s.length();
    
   int newLength = (int) stringLen - 1;   // start at last character before NULL
    
   // Find out how many trailing characters we have
   while ((0 <= newLength) && (s[newLength] == strip)) {
      --newLength;
   }
    
   ++newLength;
    
   // Did we not have any characters to strip?
   if (newLength == stringLen) {
      return s;
   }
   
   s = s.substr(0,newLength);
   
   return s;
}

//******************************************************************************

std::string& StrUtils::stripLeading(std::string& s, char stripChar) noexcept
{
   if (s.empty()) {
      return s;
   }
   
   const std::string::size_type stringLen = s.length();
   std::string::size_type leadingStripChars = 0;
   
   while ((leadingStripChars < stringLen) && (s[leadingStripChars] == stripChar)) {
      ++leadingStripChars;
   }
    
   // Any leading characters to strip?
   if (leadingStripChars > 0) {
      s = s.substr(leadingStripChars);
   }
   
   return s;
}

//******************************************************************************

std::string& StrUtils::trimLeadingSpaces(std::string& s) noexcept
{
   const std::string::size_type posFirstNonSpace = s.find_first_not_of(SPACE);
    
   if ((std::string::npos != posFirstNonSpace) && (posFirstNonSpace > 0)) {
      s.erase(0, posFirstNonSpace);
   }
    
   return s;
}

//******************************************************************************

std::string StrUtils::strip(const std::string& s) noexcept
{
   return StrUtils::strip(s, ' ');
}

//******************************************************************************

std::string StrUtils::strip(const std::string& s, char strip) noexcept
{
   if (s.empty()) {
      return EMPTY;
   }
   
   const std::string::size_type len = s.length();
    
   std::string::size_type leadingChars = 0;
    
   for (std::string::size_type i = 0; i < len; ++i) {
      if (s[i] == strip) {
         ++leadingChars;
      } else {
         break;
      }
   }
    
   if (leadingChars == len) {
      return std::string(EMPTY);
   }
    
   int trailingChars = 0;
    
   for (int i = (int) len - 1; i >= 0; --i) {
      if (s[i] == strip) {
         ++trailingChars;
      } else {
         break;
      }
   }
    
   return s.substr(leadingChars, len - trailingChars);
}

//******************************************************************************

std::string& StrUtils::replaceAll(std::string& s,
                             const std::string& searchFor,
                             const std::string& replaceWith) noexcept
{
   std::string::size_type posSearchFor = s.find(searchFor);
    
   if (posSearchFor == std::string::npos) {
      return s;
   }
    
   const std::string::size_type lenSearchFor = searchFor.length();
   const std::string::size_type lenReplaceWith = replaceWith.length();
    
   do {
      s.replace(posSearchFor, lenSearchFor, replaceWith);
      posSearchFor = s.find(searchFor, posSearchFor + lenReplaceWith);
   } while (posSearchFor != std::string::npos);
    
   return s;
}

//******************************************************************************

std::string StrUtils::gzipCompress(const std::string& str)
{
   z_stream zs;
   memset(&zs, 0, sizeof(zs));

   zs.zalloc = Z_NULL;
   zs.zfree = Z_NULL;
   zs.opaque = Z_NULL;
   zs.total_out = 0;
   zs.next_in = (Bytef*)str.data();
   zs.avail_in = str.size();           // set the z_stream's input

   // compression levels
   //Z_NO_COMPRESSION
   //Z_BEST_SPEED
   //Z_BEST_COMPRESSION
   //Z_DEFAULT_COMPRESSION

   const int compressionLevel = Z_DEFAULT_COMPRESSION;
   
   const int windowBits = 15;
   const int GZIP_ENCODING = 16;
   int rc;
   
   rc = deflateInit2(&zs,
                     compressionLevel,
                     Z_DEFLATED,
                     windowBits | GZIP_ENCODING,
                     8,
                     Z_DEFAULT_STRATEGY);
   
   if (rc != Z_OK) {
      throw std::runtime_error("deflateInit2 failed while compressing");
   }
   
   char outbuffer[16384];
   std::string outstring;
   std::string::size_type outstringSize = 0;
   std::string::size_type blockDataSize = 0;
   
   // retrieve the compressed bytes blockwise
   do {
      zs.next_out = reinterpret_cast<Bytef*>(outbuffer);
      zs.avail_out = sizeof(outbuffer);
      
      rc = deflate(&zs, Z_FINISH);
      
      if ((rc != Z_OK) && (rc != Z_STREAM_END)) {
         throw std::runtime_error("deflate failed while compressing");
      }
      
      if (outstringSize < zs.total_out) {
         // append the block to the output string
         blockDataSize = zs.total_out - outstringSize;
         outstring.append(outbuffer,
                          blockDataSize);
         outstringSize += blockDataSize;
      }
   } while (zs.avail_out == 0);
   
   rc = deflateEnd(&zs);

   if (rc != Z_OK) {
      throw std::runtime_error("deflateEnd failed while compressing");
   }

   return outstring;
}

//******************************************************************************

std::string StrUtils::gzipDecompress(const std::string& str)
{
   z_stream zs;                        // z_stream is zlib's control structure
   memset(&zs, 0, sizeof(zs));
   
   if (inflateInit(&zs) != Z_OK) {
      throw(std::runtime_error("inflateInit failed while decompressing."));
   }
   
   zs.next_in = (Bytef*)str.data();
   zs.avail_in = str.size();
   
   int ret;
   char outbuffer[32768];
   std::string outstring;
   
   // get the decompressed bytes blockwise using repeated calls to inflate
   do {
      zs.next_out = reinterpret_cast<Bytef*>(outbuffer);
      zs.avail_out = sizeof(outbuffer);
      
      ret = inflate(&zs, 0);
      
      if (outstring.size() < zs.total_out) {
         outstring.append(outbuffer,
                          zs.total_out - outstring.size());
      }
      
   } while (ret == Z_OK);
   
   inflateEnd(&zs);
   
   if (ret != Z_STREAM_END) {          // an error occurred that was not EOF
      std::ostringstream oss;
      oss << "Exception during zlib decompression: (" << ret << ") "
      << zs.msg;
      throw(std::runtime_error(oss.str()));
   }
   
   return outstring;
}

//******************************************************************************

