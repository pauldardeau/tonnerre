# Copyright Paul Dardeau, SwampBits LLC 2014
# BSD License

CC = c++
CC_OPTS = -c -pthread -I../src -I../chaudiere/src
UT_CC_OPTS = -c -std=c++20 -pthread -I../src -I../chaudiere/src -I../poivre

CLIENT_EXE = TestClient
SERVER_EXE = TestServer
UNIT_TESTS_EXE = test_tonnerre
LIB_NAMES = ../src/tonnerre.so ../chaudiere/src/libchaudiere.so
UT_LIBS = $(LIB_NAMES)
STD_LINK_LIBS = -lpthread -ldl

CLIENT_EXE_OBJS = TestClient.o

SERVER_EXE_OBJS = TestServer.o

POIVRE_OBJS = TestCase.o \
TestSuite.o

UNIT_TESTS_EXE_OBJS = Tests.o TestMessaging.o TestMessagingServer.o TestMessage.o TestMessageRequestHandler.o TestMessageSocketServiceHandler.o $(POIVRE_OBJS)

all : $(CLIENT_EXE) $(SERVER_EXE) $(UNIT_TESTS_EXE)

clean :
	rm -f *.o
	rm -f $(CLIENT_EXE)
	rm -f $(SERVER_EXE)
	rm -f $(UNIT_TESTS_EXE)

$(CLIENT_EXE) : $(CLIENT_EXE_OBJS)
	$(CC) $(CLIENT_EXE_OBJS) -o $(CLIENT_EXE) $(LIB_NAMES) $(STD_LINK_LIBS)

$(SERVER_EXE) : $(SERVER_EXE_OBJS)
	$(CC) $(SERVER_EXE_OBJS) -o $(SERVER_EXE) $(LIB_NAMES) $(STD_LINK_LIBS)

$(UNIT_TESTS_EXE) : $(UNIT_TESTS_EXE_OBJS)
	$(CC) $(UNIT_TESTS_EXE_OBJS) -o $(UNIT_TESTS_EXE) $(UT_LIBS) $(STD_LINK_LIBS)

$(POIVRE_OBJS) : %.o : ../poivre/%.cpp
	$(CC) $(UT_CC_OPTS) $< -o $@

TestClient.o TestServer.o : %.o : %.cpp
	$(CC) $(CC_OPTS) $< -o $@

%.o : %.cpp
	$(CC) $(UT_CC_OPTS) $< -o $@
