// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef HttpServer_Mutex_h
#define HttpServer_Mutex_h

/**
 * Mutex is an interface for working with abstract mutexes.
 */
class Mutex
{
public:
   /**
    * Default constructor
    */
   Mutex() noexcept {}
   
   /**
    * Destructor
    */
   virtual ~Mutex() noexcept {}
   
   /**
    *
    * @return
    */
   virtual bool unlock() noexcept = 0;
   
   /**
    *
    * @return
    */
   virtual bool lock() noexcept = 0;
   
   /**
    *
    * @return
    */
   virtual bool isLocked() const noexcept = 0;
   
   /**
    *
    * @return
    */
   virtual bool haveValidMutex() const noexcept = 0;

   
   // copying not allowed
   Mutex(const Mutex&) = delete;
   Mutex(Mutex&&) = delete;
   Mutex& operator=(const Mutex&) = delete;
   Mutex& operator=(Mutex&&) = delete;
   
   
};


#endif
