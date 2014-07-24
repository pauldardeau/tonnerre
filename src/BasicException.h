// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef C10KServer_BasicException_h
#define C10KServer_BasicException_h

#include <string>
#include <exception>


/**
 * The BasicException is a subclass of std::exception that contains
 * the reason for the exception as a std::string.
 */
class BasicException : public std::exception
{
public:
   /**
    * Constructs a BasicException
    * @param what the reason for the exception
    */
   explicit BasicException(const char* what) noexcept :
      m_what( what )
   {
   }

   /**
    * Constructs a BasicException
    * @param what the reason for the exception
    */
   explicit BasicException(const std::string& what) noexcept :
      m_what( what )
   {
   }

   /**
    * Copy constructor
    * @param copy the source of the copy
    */
   BasicException(const BasicException& copy) noexcept :
      m_what( copy.m_what )
   {
   }

   /**
    * Move constructor
    * @param move the source of the move
    */
   BasicException(BasicException&& move) noexcept :
      m_what(std::move(move.m_what))
   {
   }

   /**
    * Destructor
    */
   ~BasicException() noexcept {}

   /**
    * Copy operator
    * @param copy the source of the copy
    * @return reference to the updated instance
    */
   BasicException& operator=(const BasicException& copy) noexcept
   {
      if (this == &copy) {
         return *this;
      }
        
      m_what = copy.m_what;
        
      return *this;
   }

   /**
    * Move operator
    * @param move the source of the move
    * @return reference to the target of the move
    */
   BasicException& operator=(BasicException&& move) noexcept
   {
      if (this == &move) {
         return *this;
      }
        
      m_what = std::move(move.m_what);
        
      return *this;
   }

   /**
    * Retrieves the C string reason for the exception
    * @return C string reason for exception
    */
   virtual const char* what() const noexcept
   {
      return m_what.c_str();
   }

   /**
    * Retrieves the string reason for the exception
    * @return string reason for the exception
    */
   virtual const std::string& whatString() const noexcept
   {
      return m_what;
   }
    
private:
   std::string m_what;
};


#endif
