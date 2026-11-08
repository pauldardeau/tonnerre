# Copyright Paul Dardeau, SwampBits LLC 2014
# BSD License

CC = c++
CC_OPTS = -c -std=c++20 -Wall -fPIC -O2 -I../chaudiere/src

LIB_NAME = tonnerre.so
CHAUDIERE_LIB = ../chaudiere/src/libchaudiere.so

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
	$(CC) -shared -fPIC $(OBJS) $(CHAUDIERE_LIB) -o $(LIB_NAME)

%.o : %.cpp
	$(CC) $(CC_OPTS) $< -o $@
