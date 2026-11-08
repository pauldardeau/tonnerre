# Copyright Paul Dardeau, SwampBits LLC 2014
# BSD License

CC = c++
CC_OPTS = -c -std=c++20 -Wall -fPIC -O2 -I../chaudiere/src

UNAME_S := $(shell uname -s)

LIB_NAME = tonnerre.so
CHAUDIERE_LIB = ../chaudiere/src/libchaudiere.so

# What the link rule below uses for Chaudiere on every platform except Darwin (which has its own branch): the library by pathname.
SO_LINK_LIBS = $(CHAUDIERE_LIB)

ifeq ($(UNAME_S),Linux)
# An ELF dependent records a library without a SONAME by the path it was linked from, and a name with a slash is looked up
# relative to the current directory. So Chaudiere is linked with -L/-l (recorded as libchaudiere.so), tonnerre.so is given the
# SONAME tonnerre.so (so that its dependents record that bare name), and a RUNPATH relative to the library itself ($$ORIGIN,
# which make turns into $ORIGIN, inside single quotes for the shell) lets the loader find Chaudiere wherever tonnerre.so
# is loaded from. libchaudiere.so itself is not changed.
SO_LINK_LIBS = -L../chaudiere/src -lchaudiere -Wl,-soname,$(LIB_NAME) -Wl,-rpath,'$$ORIGIN/../chaudiere/src'
endif

ifeq ($(UNAME_S),Darwin)
# A Mach-O dependent records the install name of a library, not the path it was linked from, and dyld
# finds a bare name only in the current directory and the default library directories. So tonnerre.so is
# given the install name @rpath/tonnerre.so and a run path relative to itself, and the name that
# libchaudiere.so carries is replaced, in tonnerre.so, by @rpath/libchaudiere.so. libchaudiere.so itself
# is not changed.
SO_LINK_OPTS = -Wl,-install_name,@rpath/$(LIB_NAME) -Wl,-rpath,@loader_path/../chaudiere/src

# A library whose post-link step failed must not stay behind for a later make to take as built.
.DELETE_ON_ERROR:

# Makes dependent $(2) record library $(1) as @rpath/<file name of $(1)>, whatever install name the built
# library carries, and fails unless it then records exactly that. (install_name_tool -change alone ignores
# an old name that is not there.)
define RECORD_RPATH_NAME
old=$$(otool -D $(1) | tail -n 1) && new=@rpath/$$(basename $(1)) && \
{ [ "$$old" = "$$new" ] || install_name_tool -change "$$old" "$$new" $(2); } && \
otool -L $(2) | grep -F -q "$$new (" || { echo "error: $(2) does not record $$new" >&2; exit 1; }
endef
endif

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
ifeq ($(UNAME_S),Darwin)
	$(CC) -shared -fPIC $(OBJS) $(CHAUDIERE_LIB) $(SO_LINK_OPTS) -o $(LIB_NAME)
	$(call RECORD_RPATH_NAME,$(CHAUDIERE_LIB),$(LIB_NAME))
else
	$(CC) -shared -fPIC $(OBJS) $(SO_LINK_LIBS) -o $(LIB_NAME)
endif

%.o : %.cpp
	$(CC) $(CC_OPTS) $< -o $@
