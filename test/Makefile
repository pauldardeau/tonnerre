# Copyright Paul Dardeau, SwampBits LLC 2014
# BSD License

CC = c++
CC_OPTS = -c -std=c++20 -pthread -I../src -I../chaudiere/src
UT_CC_OPTS = -c -std=c++20 -pthread -I../src -I../chaudiere/src -I../poivre

UNAME_S := $(shell uname -s)

CLIENT_EXE = TestClient
SERVER_EXE = TestServer
UNIT_TESTS_EXE = test_tonnerre
LIB_NAMES = ../src/tonnerre.so ../chaudiere/src/libchaudiere.so
UT_LIBS = $(LIB_NAMES)
STD_LINK_LIBS = -lpthread -ldl

ifeq ($(UNAME_S),Darwin)
# A Mach-O dependent records the install name of a library, not the path it was linked from (see src/Makefile).
# tonnerre.so carries the install name @rpath/tonnerre.so; the name libchaudiere.so carries is replaced, in each
# executable, by @rpath/libchaudiere.so. The run paths are relative to the executable, so the executables
# load their libraries whatever the current directory, and when the tree is moved as a whole.
EXE_RPATHS = -Wl,-rpath,@loader_path/../src -Wl,-rpath,@loader_path/../chaudiere/src

# An executable whose post-link step failed must not stay behind for a later make to take as built.
.DELETE_ON_ERROR:

# Makes dependent $(2) record library $(1) as @rpath/<file name of $(1)>, whatever install name the built
# library carries, and fails unless it then records exactly that. (install_name_tool -change alone ignores
# an old name that is not there.) Same as in src/Makefile.
define RECORD_RPATH_NAME
old=$$(otool -D $(1) | tail -n 1) && new=@rpath/$$(basename $(1)) && \
{ [ "$$old" = "$$new" ] || install_name_tool -change "$$old" "$$new" $(2); } && \
otool -L $(2) | grep -F -q "$$new (" || { echo "error: $(2) does not record $$new" >&2; exit 1; }
endef
endif

CLIENT_EXE_OBJS = TestClient.o

SERVER_EXE_OBJS = TestServer.o

POIVRE_OBJS = TestCase.o \
TestSuite.o

UNIT_TESTS_EXE_OBJS = Tests.o TestMessaging.o TestMessagingServer.o TestMessage.o TestMessageRequestHandler.o TestMessageSocketServiceHandler.o IsolatedCase.o TestReceivePath.o $(POIVRE_OBJS)

all : $(CLIENT_EXE) $(SERVER_EXE) $(UNIT_TESTS_EXE)

clean :
	rm -f *.o
	rm -f $(CLIENT_EXE)
	rm -f $(SERVER_EXE)
	rm -f $(UNIT_TESTS_EXE)

$(CLIENT_EXE) : $(CLIENT_EXE_OBJS)
ifeq ($(UNAME_S),Darwin)
	$(CC) $(CLIENT_EXE_OBJS) -o $(CLIENT_EXE) $(LIB_NAMES) $(STD_LINK_LIBS) $(EXE_RPATHS)
	$(call RECORD_RPATH_NAME,../src/tonnerre.so,$(CLIENT_EXE))
	$(call RECORD_RPATH_NAME,../chaudiere/src/libchaudiere.so,$(CLIENT_EXE))
else
	$(CC) $(CLIENT_EXE_OBJS) -o $(CLIENT_EXE) $(LIB_NAMES) $(STD_LINK_LIBS)
endif

$(SERVER_EXE) : $(SERVER_EXE_OBJS)
ifeq ($(UNAME_S),Darwin)
	$(CC) $(SERVER_EXE_OBJS) -o $(SERVER_EXE) $(LIB_NAMES) $(STD_LINK_LIBS) $(EXE_RPATHS)
	$(call RECORD_RPATH_NAME,../src/tonnerre.so,$(SERVER_EXE))
	$(call RECORD_RPATH_NAME,../chaudiere/src/libchaudiere.so,$(SERVER_EXE))
else
	$(CC) $(SERVER_EXE_OBJS) -o $(SERVER_EXE) $(LIB_NAMES) $(STD_LINK_LIBS)
endif

$(UNIT_TESTS_EXE) : $(UNIT_TESTS_EXE_OBJS)
ifeq ($(UNAME_S),Darwin)
	$(CC) $(UNIT_TESTS_EXE_OBJS) -o $(UNIT_TESTS_EXE) $(UT_LIBS) $(STD_LINK_LIBS) $(EXE_RPATHS)
	$(call RECORD_RPATH_NAME,../src/tonnerre.so,$(UNIT_TESTS_EXE))
	$(call RECORD_RPATH_NAME,../chaudiere/src/libchaudiere.so,$(UNIT_TESTS_EXE))
else
	$(CC) $(UNIT_TESTS_EXE_OBJS) -o $(UNIT_TESTS_EXE) $(UT_LIBS) $(STD_LINK_LIBS)
endif

$(POIVRE_OBJS) : %.o : ../poivre/%.cpp
	$(CC) $(UT_CC_OPTS) $< -o $@

TestClient.o TestServer.o : %.o : %.cpp
	$(CC) $(CC_OPTS) $< -o $@

%.o : %.cpp
	$(CC) $(UT_CC_OPTS) $< -o $@
