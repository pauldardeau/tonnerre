// Copyright Paul Dardeau, SwampBits LLC 2014
// BSD License

#ifndef C10KServer_Socket_h
#define C10KServer_Socket_h

#include <netinet/in.h>
#include <sys/types.h>
#include <string>
#include <memory>

class Socket;
class SocketCompletionObserver;

/*!
 * Socket is very similar to Java's Socket class. It provides a wrapper class
 * for working with sockets.
 */
class Socket : public std::enable_shared_from_this<Socket>
{
public:
   
   static int createSocket() noexcept;
   
   // throws BasicException
   Socket(const std::string& address, int port);
   explicit Socket(int socketFD) noexcept;
   Socket(SocketCompletionObserver* completionObserver, int socketFD) noexcept;
   ~Socket() noexcept;
    
   ssize_t send(const void* sendBuffer, size_t bufferLength, int flags) noexcept;
   bool write(const char* buffer, unsigned long bufsize) noexcept;
   bool write(const std::string& payload) noexcept;

    
   ssize_t receive(void* receiveBuffer, size_t bufferLength, int flags) noexcept;
   bool read(char* buffer, int bufferLen) noexcept;
    
   void close() noexcept;
   bool isOpen() const noexcept;
   bool isConnected() const noexcept;
   void closeConnection() noexcept;
   int getFileDescriptor() const noexcept;
   void requestComplete() noexcept;
    
   void setUserIndex(int userIndex) noexcept;
   int getUserIndex() const noexcept;
    
   bool setTcpNoDelay(bool on) noexcept;
   bool getTcpNoDelay() const noexcept;
    
   bool setSendBufferSize(int size) noexcept;
   int getSendBufferSize() const noexcept;
    
   bool setReceiveBufferSize(int size) noexcept;
   int getReceiveBufferSize() const noexcept;
    
   bool setKeepAlive(bool on) noexcept;
   bool getKeepAlive() const noexcept;
    
   bool readLine(std::string& line) noexcept;
    
   bool getPeerIPAddress(std::string& ipAddress) noexcept;
    
   int getPort() const noexcept;
    
   void setIncludeMessageSize(bool isSizeIncluded) noexcept;


   // copying not allowed
   Socket(const Socket&) = delete;
   Socket(Socket&&) = delete;
   Socket& operator=(const Socket&) = delete;
   Socket& operator=(Socket&&) = delete;

   
protected:
   bool readMsg(int length) noexcept;
   bool readSocket(char* buffer, int bytesToRead) noexcept;
   bool open() noexcept;
   void init() noexcept;
   
   void setLineInputBuffer(const std::string& s) noexcept;
   void appendLineInputBuffer(const std::string& s) noexcept;

    
private:
   SocketCompletionObserver* m_completionObserver;
   std::string m_lineInputBuffer;
   std::string m_serverAddress;
   struct sockaddr_in m_serverAddr;
   int m_socketFD;
   int m_userIndex;
   int m_port;
   bool m_isConnected;
   bool m_includeMessageSize;
   std::unique_ptr<char[]> m_inputBuffer;
   int m_inBufferSize;
   int m_lastReadSize;
};


#endif
