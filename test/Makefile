# Copyright Paul Dardeau, SwampBits LLC 2014
# BSD License

CC = clang++
CC_OPTS = -c -pthread -fblocks -std=c++11 -I../src -I../../chaudiere/src

CLIENT = TestClient
SERVER = TestServer
LIB_NAME = libtonnerre.a

CLIENT_OBJS = TestClient.o

SERVER_OBJS = TestServer.o

all : $(CLIENT) $(SERVER)

clean :
	rm -f *.o
	rm -f $(CLIENT)
	rm -f $(SERVER)

$(CLIENT) : $(CLIENT_OBJS)
	$(CC) -pthread -fblocks $(CLIENT_OBJS) ../src/$(LIB_NAME) ../../chaudiere/src/libchaudiere.a -o $(CLIENT)

$(SERVER) : $(SERVER_OBJS)
	$(CC) -pthread -fblocks $(SERVER_OBJS) ../src/$(LIB_NAME) ../../chaudiere/src/libchaudiere.a -o $(SERVER)

%.o : %.cpp
	$(CC) $(CC_OPTS) $< -o $@
