# Copyright Paul Dardeau, SwampBits LLC 2014
# BSD License

CC = g++
CC_OPTS = -c -pthread -I../src -I../../chaudiere/src

CLIENT = TestClient
SERVER = TestServer
LIB_NAME = tonnerre.so

CLIENT_OBJS = TestClient.o

SERVER_OBJS = TestServer.o

all : $(CLIENT) $(SERVER)

clean :
	rm -f *.o
	rm -f $(CLIENT)
	rm -f $(SERVER)

$(CLIENT) : $(CLIENT_OBJS)
	$(CC) $(CLIENT_OBJS) -o $(CLIENT) ../../chaudiere/src/chaudiere.so ../src/$(LIB_NAME) -lpthread -ldl

$(SERVER) : $(SERVER_OBJS)
	$(CC) $(SERVER_OBJS) -o $(SERVER) ../../chaudiere/src/chaudiere.so ../src/$(LIB_NAME) -lpthread -ldl

%.o : %.cpp
	$(CC) $(CC_OPTS) $< -o $@
