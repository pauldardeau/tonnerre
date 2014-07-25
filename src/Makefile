CC = clang++
CC_OPTS = -c -std=c++11
ARCHIVE_CMD = ar
ARCHIVE_OPTS = rs

LIB_NAME = libtonnerre.a

OBJS = EpollServer.o \
IniReader.o \
InvalidKeyException.o \
KernelEventServer.o \
KeyValuePairs.o \
KqueueServer.o \
Logger.o \
Message.o \
MessageRequestHandler.o \
Messaging.o \
MessagingServer.o \
PthreadsConditionVariable.o \
PthreadsMutex.o \
PthreadsThread.o \
RequestHandler.o \
ServerSocket.o \
ServiceInfo.o \
Socket.o \
SocketRequest.o \
SocketServer.o \
StdConditionVariable.o \
StdLogger.o \
StdMutex.o \
StdThread.o \
StrUtils.o \
StringTokenizer.o \
Thread.o \
ThreadPool.o \
ThreadPoolDispatch.o \
ThreadPoolQueue.o \
ThreadPoolWorker.o \
ThreadingFactory.o

all : $(LIB_NAME)

clean :
	rm -f *.o
	rm -f $(LIB_NAME)

$(LIB_NAME) : $(OBJS)
	$(ARCHIVE_CMD) $(ARCHIVE_OPTS) $(LIB_NAME) $(OBJS)

%.o : %.cpp
	$(CC) $(CC_OPTS) $< -o $@
