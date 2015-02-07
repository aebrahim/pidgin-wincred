#
# Makefile.mingw
#
# Description: Makefile for win32 (mingw) version of libpurple Plugins
#

TARGET = pidgin-wincred.dll

PIDGIN_TREE_TOP := ../..
include $(PIDGIN_TREE_TOP)/libpurple/win32/global.mak
.SUFFIXES: .c .dll

##
## INCLUDE PATHS
##
INCLUDE_PATHS +=	\
			-I$(GTK_TOP)/include \
			-I$(GTK_TOP)/include/glib-2.0 \
			-I$(GTK_TOP)/lib/glib-2.0/include \
			-I$(PIDGIN_TREE_TOP) \
			-I$(PURPLE_TOP) \
			-I$(PURPLE_TOP)/win32 \
			-I`pwd`

LIB_PATHS +=		\
			-L$(GTK_TOP)/lib \
			-L$(PURPLE_TOP)

##
## LIBRARIES
##
LIBS =	\
			-lglib-2.0 \
			-lgobject-2.0 \
			-lgmodule-2.0 \
			-lintl \
			-lws2_32 \
			-lpurple \

CFLAGS = -O2 -Wall -Waggregate-return -Wcast-align -Wdeclaration-after-statement -Werror-implicit-function-declaration -Wextra -Wno-sign-compare -Wno-unused-parameter -Winit-self -Wmissing-declarations -Wmissing-prototypes -Wnested-externs -Wpointer-arith -Wundef -pipe -mms-bitfields -g

##
## TARGET DEFINITIONS
##
.PHONY: all clean

all: $(TARGET)

.c.dll:
	$(CC) $(CFLAGS) $(DEFINES) $(INCLUDE_PATHS) -mwindows -o $@.o -c $<
	$(CC) -shared $@.o $(LIB_PATHS) $(LIBS) $(DLL_LD_FLAGS) -o $@
	rm $@.o

install: $(TARGET)
	cp $(TARGET) /cygdrive/c/Users/$(USERNAME)/AppData/Roaming/.purple/plugins/



##
## CLEAN RULES
##
clean:
	rm -f *.o *.dll

include $(PIDGIN_COMMON_TARGETS)
