// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <cerrno>

#include "Socket.h"
#include "SocketCompletionObserver.h"
#include "BasicException.h"
#include "Logger.h"

static const std::string EMPTY_STRING = "";
static const std::string CRLF = "\r\n";
static const std::string LF = "\n";

static const int DEFAULT_BUFFER_SIZE = 1024;


//******************************************************************************

int Socket::createSocket() noexcept
{
   const int socketFD = ::socket(AF_INET, SOCK_STREAM, 0);
   if (socketFD == -1) {
      Logger::error("unable to create socket");
   }
   
   return socketFD;
}

//******************************************************************************

Socket::Socket(const std::string& address, int port) :
   m_completionObserver(nullptr),
   m_serverAddress(address),
   m_socketFD(-1),
   m_userIndex(-1),
   m_port(port),
   m_isConnected(false),
   m_includeMessageSize(false),
   m_inputBuffer(nullptr),
   m_inBufferSize(DEFAULT_BUFFER_SIZE)
{
   Logger::logInstanceCreate("Socket");

   if (!open()) {
      throw BasicException("Unable to open socket");
   }
}

//******************************************************************************

Socket::Socket(int socketFD) noexcept :
   Socket(nullptr, socketFD)
{
}

//******************************************************************************

Socket::Socket(SocketCompletionObserver* completionObserver, int socketFD) noexcept :
   m_completionObserver(completionObserver),
   m_socketFD(socketFD),
   m_userIndex(-1),
   m_isConnected(true),
   m_includeMessageSize(false),
   m_inputBuffer(new char[DEFAULT_BUFFER_SIZE]),
   m_inBufferSize(DEFAULT_BUFFER_SIZE)
{
   Logger::logInstanceCreate("Socket");
}

//******************************************************************************

Socket::~Socket() noexcept
{
   Logger::logInstanceDestroy("Socket");
}

//******************************************************************************

void Socket::init() noexcept
{
   setTcpNoDelay(true);
}

//******************************************************************************

bool Socket::open() noexcept
{
   m_socketFD = Socket::createSocket();

   if (m_socketFD < 0) {
      Logger::error("unable to open socket connection");
      return false;
   }

   int rc = 0;

   ::memset((void *) &m_serverAddr, 0, sizeof(m_serverAddr));

   m_serverAddr.sin_family = AF_INET;
   m_serverAddr.sin_port = htons(m_port);
   m_serverAddr.sin_addr.s_addr = inet_addr(m_serverAddress.c_str());

   rc = ::connect(m_socketFD,
                  (struct sockaddr *) &m_serverAddr,
                  sizeof(m_serverAddr));

   if (rc < 0) {
      return false;
   } else {
      init();
      return true;
   }
}

//******************************************************************************

ssize_t Socket::send(const void* sendBuffer, size_t bufferLength, int flags) noexcept
{
   if ((m_socketFD < 0) ||
       (! m_isConnected) ||
       (nullptr == sendBuffer)) {
      return -1;
   }
    
   return ::send(m_socketFD,sendBuffer,bufferLength,flags);
}

//******************************************************************************

ssize_t Socket::receive(void* receiveBuffer, size_t bufferLength, int flags) noexcept
{
   if ((m_socketFD < 0) ||
       (!m_isConnected) ||
       (nullptr == receiveBuffer)) {
      return -1;
   }
    
   const ssize_t bytesReceived =
      ::recv(m_socketFD,receiveBuffer,bufferLength,flags);
    
   if (0 == bytesReceived) {
      //::close(m_socketFD);
      m_socketFD = -1;
      m_isConnected = false;
   }
    
   return bytesReceived;
}

//******************************************************************************

void Socket::closeConnection() noexcept
{
   close();
}

//******************************************************************************

void Socket::close() noexcept
{
   if (m_socketFD > -1) {
      ::close(m_socketFD);
      m_socketFD = -1;
      m_isConnected = false;
   }
}

//******************************************************************************

bool Socket::isConnected() const noexcept
{
   return m_isConnected;
}

//******************************************************************************

int Socket::getFileDescriptor() const noexcept
{
   return m_socketFD;
}

//******************************************************************************

void Socket::requestComplete() noexcept
{
   if (m_completionObserver) {
      m_completionObserver->notifySocketComplete(shared_from_this());
   }
}

//******************************************************************************

void Socket::setUserIndex(int userIndex) noexcept
{
   m_userIndex = userIndex;
}

//******************************************************************************

int Socket::getUserIndex() const noexcept
{
   return m_userIndex;
}

//******************************************************************************

bool Socket::setTcpNoDelay(bool on) noexcept
{
   int sockopt_arg = on ? 1 : 0;
   return (0 == ::setsockopt(m_socketFD,
               IPPROTO_TCP,
               TCP_NODELAY,
               (char *) &sockopt_arg,
               sizeof(sockopt_arg)));
}

//******************************************************************************

bool Socket::getTcpNoDelay() const noexcept
{
   int result = 0;
   socklen_t len = sizeof(int);
    
   if (0 == ::getsockopt(m_socketFD,
                        IPPROTO_TCP,
                        TCP_NODELAY,
                        (char*) &result,
                        &len)) {
      return (result > 0 ? true : false);
   } else {
      // error
      return false;
   }
}

//******************************************************************************

bool Socket::setSendBufferSize(int size) noexcept
{
   int sockopt_arg = size;
   return (0 == ::setsockopt(m_socketFD,
               SOL_SOCKET,
               SO_SNDBUF,
               (char *) &sockopt_arg,
               sizeof(sockopt_arg)));
}

//******************************************************************************

int Socket::getSendBufferSize() const noexcept
{
   int result = 0;
   socklen_t len = sizeof(int);
    
   if (0 == ::getsockopt(m_socketFD,
                        SOL_SOCKET,
                        SO_SNDBUF,
                        (char*) &result,
                        &len)) {
      return result;
   } else {
      // error
      return -1;
   }
}

//******************************************************************************

bool Socket::setReceiveBufferSize(int size) noexcept
{
   int sockopt_arg = size;
   return (0 == ::setsockopt(m_socketFD,
               SOL_SOCKET,
               SO_RCVBUF,
               (char *) &sockopt_arg,
               sizeof(sockopt_arg)));
}

//******************************************************************************

int Socket::getReceiveBufferSize() const noexcept
{
   int result = 0;
   socklen_t len = sizeof(int);
    
   if (0 == ::getsockopt(m_socketFD,
                        SOL_SOCKET,
                        SO_RCVBUF,
                        (char*) &result,
                        &len)) {
      return result;
   } else {
      // error
      return -1;
   }
}

//******************************************************************************

bool Socket::setKeepAlive(bool on) noexcept
{
   int sockopt_arg = on ? 1 : 0;
   return (0 == ::setsockopt(m_socketFD,
               SOL_SOCKET,
               SO_KEEPALIVE,
               (char *) &sockopt_arg,
               sizeof(sockopt_arg)));
}

//******************************************************************************

bool Socket::getKeepAlive() const noexcept
{
   int result = 0;
   socklen_t len = sizeof(int);
    
   if (0 == ::getsockopt(m_socketFD,
                        SOL_SOCKET,
                        SO_KEEPALIVE,
                        (char*) &result,
                        &len)) {
      return (result > 0 ? true : false);
   } else {
      // error
      return false;
   }
}

//******************************************************************************

bool Socket::readLine(std::string& line) noexcept
{
   line.erase();
    
   int eolLength = 0;
   std::string::size_type posEOL = std::string::npos;
    
   // do we already have anything in our line input buffer?
   if (!m_lineInputBuffer.empty()) {
      // look for CRLF EOL
      const std::string::size_type posCRLF = m_lineInputBuffer.find(CRLF);
        
      // is there one?
      if (std::string::npos != posCRLF) {
         eolLength = 2;
         posEOL = posCRLF;
      } else {
         const std::string::size_type posLF = m_lineInputBuffer.find(LF);
            
         if (std::string::npos != posLF) {
            eolLength = 1;
            posEOL = posLF;
         }
      }
        
      // did we find the EOL?
      if ((eolLength > 0) && (std::string::npos != posEOL)) {
         // extract everything prior to EOL
         line = m_lineInputBuffer.substr(0, posEOL);
            
         // remove the current line from the line input buffer
         m_lineInputBuffer.erase(0, posEOL + eolLength);
         
         if (Logger::isLogging(Logger::LogLevel::Debug)) {
            Logger::debug("Socket::readLine, using buffer line: (next line)");
            Logger::debug(line);
            
            Logger::debug("just called erase to remove current line from input buffer");
            Logger::debug("*** remaining input buffer on next line");
         }
         
         return true;
      } else {
         Logger::debug("*** assigning full input buffer");
         
         // put our line input buffer contents into the current line
         line = m_lineInputBuffer;
         
         if (Logger::isLogging(Logger::LogLevel::Debug)) {
            Logger::debug("Socket::readLine, using partial buffer: (next line)");
            Logger::debug(line);
         }
            
         // empty our line input buffer
         m_lineInputBuffer.erase();
      }
   }
    
   ssize_t bytes = 0;
   int bytesToRead = 1023;
   char buffer[1024];
   char* posLF;
   char* posCRLF;
   char* pszEOL;
   eolLength = 0;
    
   do {
      posLF = nullptr;
      posCRLF = nullptr;
      pszEOL = nullptr;
        
      bytes = ::recv(m_socketFD,
                     buffer,
                     bytesToRead,
                     0);

      if (Logger::isLogging(Logger::LogLevel::Debug)) {
         char msg[128];
         std::snprintf(msg, 128, "readline: bytes from recv = %ld", bytes);
         Logger::debug(std::string(msg));
         buffer[bytes] = '\0';
         Logger::debug("just read from socket recv (next line)");
         Logger::debug(std::string(buffer));
      }
        
      if (bytes <= 0) {  // error or connection closed by peer?
         if (bytes == 0) {
            // our connection has been closed by the other process. nothing
            // more we can do!!!
            Logger::debug("connection reset by peer");
            close();
            return false;
         } else {
            if (EINTR == errno) {  // interrupted?
               // not really an error
               continue;
            } else {
               Logger::warning("recv returned an error");
            }
         }
         return false;
      }
        
      buffer[bytes] = '\0';
        
      posCRLF = ::strstr(buffer, "\r\n");
        
      if (posCRLF) {
         pszEOL = posCRLF;
         eolLength = 2;
      } else {
         posLF = ::strstr(buffer, "\n");
            
         if (posLF) {
            pszEOL = posLF;
            eolLength = 1;
         }
      }
        
      if ((nullptr != pszEOL) && (eolLength > 0)) {
         // hold onto everything after the EOL in the line input buffer
         m_lineInputBuffer = (pszEOL + eolLength);
         
         if (Logger::isLogging(Logger::LogLevel::Debug)) {
            Logger::debug("Socket::readLine holding onto following text in line input buffer");
            char msg[256];
            std::snprintf(msg, 256, "buffer: '%s'", m_lineInputBuffer.c_str());
            Logger::debug(std::string(msg));
         }
            
         // terminate the string at the EOL
         *pszEOL = '\0';
      }
        
      line += buffer;
        
   } while (nullptr == pszEOL);
   
   return true;
}

//******************************************************************************

bool Socket::read(char* buffer, int bufsize) noexcept
{
   int length;
   int bytesAlreadyRead = 0;
    
   if (!m_lineInputBuffer.empty()) {
      const int nInputBufferLen = (int) m_lineInputBuffer.length();
        
      if (nInputBufferLen >= bufsize) {
         // we don't have to read from the socket, we've got the full amount
         // in our line input buffer
         ::memcpy(buffer, m_lineInputBuffer.c_str(), bufsize);
         m_lineInputBuffer.erase(0, bufsize);
         return true;
      } else {
         // empty the line input buffer
         ::memcpy(buffer, m_lineInputBuffer.c_str(), nInputBufferLen);
         m_lineInputBuffer = EMPTY_STRING;
         length = bufsize - nInputBufferLen;
         bytesAlreadyRead = nInputBufferLen;
      }
   } else {
      length = bufsize;
   }
    
   if (length <= 0) {
      Logger::warning("Socket::read failed, length <= 0");
      return (bytesAlreadyRead > 0);
   }
    
   if (bufsize < length) {
      Logger::warning("Socket::read failed, bufsize < length");
      return (bytesAlreadyRead > 0);
   }
   
   if (!readMsg(length)) {
      Logger::warning("Socket::read failed, readMsg returned false");
      return (bytesAlreadyRead > 0);
   }
    
   ::memcpy(buffer + bytesAlreadyRead, m_inputBuffer.get(), length);
    
   return true;
}

//******************************************************************************

bool Socket::readMsg(int length) noexcept
{
   if (!isOpen()) {
      Logger::warning("unable to read message size, socket is closed");
      return false;
   }
    
   // do we need a bigger buffer?
   if (((length + 1) > m_inBufferSize) || !m_inputBuffer) {
      m_inBufferSize = length + 1;
      std::unique_ptr<char []> newBuffer(new char[m_inBufferSize]);
      m_inputBuffer = std::move(newBuffer);
   }
    
   if (readSocket(m_inputBuffer.get(), length)) {
      m_inputBuffer[length] = '\0';
      m_lastReadSize = length;
      return true;
   } else {
      Logger::error("readSocket failed");
      return false;
   }
}

//******************************************************************************

bool Socket::readSocket(char* buffer, int bytesToRead) noexcept
{
   int total_bytes_rcvd = 0;
   ssize_t bytes = 0;
   char* currentBufferDest = buffer;
    
   do {
      bytes = ::recv(m_socketFD,
                     currentBufferDest,
                     bytesToRead - total_bytes_rcvd,
                     0);
      
      if (Logger::isLogging(Logger::LogLevel::Debug)) {
         char msg[128];
         std::snprintf(msg, 128, "recv, bytes from recv = %ld", bytes);
         Logger::debug(std::string(msg));
      }
        
      if (bytes <= 0) {  // error or connection closed by peer?
         if (bytes == 0) {
            // our connection has been closed by the other process. nothing
            // more we can do!!!
            Logger::debug("connection reset by peer");
            close();
            return false;
         } else {
            if (EINTR == errno) {  // interrupted?
               // not really an error
               continue;
            } else {
               Logger::warning("recv returned an error");
            }
         }
            
         return false;
      }
        
      total_bytes_rcvd += bytes;
      currentBufferDest += bytes;
        
   } while (total_bytes_rcvd < bytesToRead);
    
   return true;
}

//******************************************************************************

bool Socket::isOpen() const noexcept
{
   return (m_socketFD != -1);
}

//******************************************************************************

bool Socket::getPeerIPAddress(std::string& ipAddress) noexcept
{
   struct sockaddr_in addr;
   socklen_t x = sizeof(addr);
    
   if (!::getpeername(m_socketFD, (struct sockaddr*) &addr, &x)) {
      char ipAddressBuffer[64];
      memset(ipAddressBuffer, 0, sizeof(ipAddressBuffer));
      ::inet_ntop(AF_INET,
                  &addr.sin_addr,
                  ipAddressBuffer,
                  sizeof(ipAddressBuffer));
      ipAddress = ipAddressBuffer;
      return true;
   } else {
      return false;
   }
}

//******************************************************************************

bool Socket::write(const char* buffer, unsigned long bufsize) noexcept
{
   if (!isOpen()) {
      Logger::warning("unable to write message, socket is closed");
      return false;
   }
    
   const ssize_t rc = ::send(m_socketFD, buffer, bufsize, 0);
   if (rc < 0) {
      char msg[128];
      std::snprintf(msg, 128, "socket send failed, rc = %ld", rc);
      Logger::warning(std::string(msg));
   }
     
   return (rc < 0 ? false : true);
}

//******************************************************************************

bool Socket::write(const std::string& payload) noexcept
{
   return write(payload.c_str(), payload.length());
}

//******************************************************************************

int Socket::getPort() const noexcept
{
   return m_port;
}

//******************************************************************************

void Socket::setIncludeMessageSize(bool isSizeIncluded) noexcept
{
   m_includeMessageSize = isSizeIncluded;
}

//******************************************************************************

void Socket::setLineInputBuffer(const std::string& s) noexcept
{
   m_lineInputBuffer = s;
}

//******************************************************************************

void Socket::appendLineInputBuffer(const std::string& s) noexcept
{
   m_lineInputBuffer += s;
}

//******************************************************************************
