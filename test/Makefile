CC = clang++
CC_OPTS = -c -std=c++11 -I../src

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
	$(CC) $(CLIENT_OBJS) ../src/$(LIB_NAME) -o $(CLIENT)

$(SERVER) : $(SERVER_OBJS)
	$(CC) $(SERVER_OBJS) ../src/$(LIB_NAME) -o $(SERVER)

%.o : %.cpp
	$(CC) $(CC_OPTS) $< -o $@