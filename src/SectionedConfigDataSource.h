// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef SECTIONEDCONFIGDATASOURCE_H
#define SECTIONEDCONFIGDATASOURCE_H

#include <string>

class KeyValuePairs;

/*!
 * SectionedConfigDataSource is an interface for working with configuration
 * mechanisms based on sections, keys, and values.
 */
class SectionedConfigDataSource
{
public:
   /**
    * Destructor
    */
   virtual ~SectionedConfigDataSource() noexcept {}
   
   /**
    * Determines whether the specified section name exists in the configuration
    * @param sectionName the name of the section whose existence is being tested
    * @return boolean indicating whether the specified section exists
    */
   virtual bool hasSection(const std::string& sectionName) const noexcept = 0;
   
   /**
    * Reads the key/value pairs of the specified section
    * @param sectionName the name of the section to read
    * @param settings collection of key/value pairs to populate from section
    * @see KeyValuePairs()
    * @return boolean indicating whether the specified section could be read
    */
   virtual bool readSection(const std::string& sectionName,
                            KeyValuePairs& settings) const noexcept = 0;
   
   /**
    * Retrieves the value associated with the specified key within the specified section
    * @param section the name of the section containing the key/value pair
    * @param key the name of the key whose value is requested
    * @param value will be assigned the key/value pair value (if it exists)
    * @return boolean indicating whether the value was retrieved
    */
   virtual bool getSectionKeyValue(const std::string& section,
                                   const std::string& key,
                                   std::string& value) const noexcept = 0;

};

#endif
