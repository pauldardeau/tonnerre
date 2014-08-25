# Copyright Paul Dardeau, SwampBits LLC 2014
# BSD License

CC = clang++
CC_OPTS = -c -O2 -std=c++11 -I../../chaudiere/src
ARCHIVE_CMD = ar
ARCHIVE_OPTS = rs

LIB_NAME = libtonnerre.a

OBJS =  Message.o \
MessageRequestHandler.o \
MessageSocketServiceHandler.o \
Messaging.o \
MessagingServer.o

all : $(LIB_NAME)

clean :
	rm -f *.o
	rm -f $(LIB_NAME)

$(LIB_NAME) : $(OBJS)
	$(ARCHIVE_CMD) $(ARCHIVE_OPTS) $(LIB_NAME) $(OBJS)

%.o : %.cpp
	$(CC) $(CC_OPTS) $< -o $@
