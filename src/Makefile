# Copyright Paul Dardeau, SwampBits LLC 2014
# BSD License

CC = c++
CC_OPTS = -c -Wall -fPIC -O2 -I../chaudiere/src

LIB_NAME = tonnerre.so

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
	$(CC) -shared -fPIC $(OBJS) -o $(LIB_NAME)

%.o : %.cpp
	$(CC) $(CC_OPTS) $< -o $@
