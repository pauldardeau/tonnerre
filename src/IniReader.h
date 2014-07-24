// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef IniReader_h
#define IniReader_h

#include <string>

#include "SectionedConfigDataSource.h"

class KeyValuePairs;

/*!
 * IniReader is a utility class that knows how to read and parse a .ini
 * configuration file. .ini configuration files were popularized on Win32
 * platforms as a simple form of configuration files.
 */
class IniReader : public SectionedConfigDataSource
{
public:
   /**
    * Constructs an IniReader using the specified file
    * @param iniFile the file name/path of the INI file to read
    * @throw BasicException
    */
   explicit IniReader(const std::string& iniFile);
   
   /**
    * Destructor
    */
   virtual ~IniReader() noexcept;
   
   /**
    * Reads the key/value pairs of the specified section
    * @param section the name of the section to read
    * @param mapSectionValues collection of key/value pairs to populate from section
    * @see KeyValuePairs()
    * @return boolean indicating whether the specified section could be read
    */
   virtual bool readSection(const std::string& section,
                            KeyValuePairs& mapSectionValues) const noexcept override;

   /**
    * Retrieves the value associated with the specified key within the specified section
    * @param section the name of the section containing the key/value pair
    * @param key the name of the key whose value is requested
    * @param value will be assigned the key/value pair value (if it exists)
    * @return boolean indicating whether the value was retrieved
    */
   virtual bool getSectionKeyValue(const std::string& section,
                                   const std::string& key,
                                   std::string& value) const noexcept override;
   
   /**
    * Determines whether the specified section name exists in the INI file
    * @param section the name of the section whose existence is being tested
    * @return boolean indicating whether the specified section exists
    */
   virtual bool hasSection(const std::string& section) const noexcept override;
    
    
protected:
   /**
    * Reads the file from the filesystem (file name/path specified in constructor)
    * @return boolean indicating if the file was read successfully
    */
   bool readFile() noexcept;
   
   /**
    * Retrieves the string identifier in the INI file for the specified section name
    * @param sectionName the name of the section whose identifier is to be constructed
    * @return the section identifier for the specified section name
    */
   std::string bracketedSection(const std::string& sectionName) const noexcept;
   
    
private:
   std::string m_iniFile;
   std::string m_fileContents;
};


#endif
