# Standalone Makefile for automated builds of win32 libpurple plugins

TARGET = pidgin-wincred.dll

# Filepaths
GTK_TOP := gtk
PURPLE_TOP := pidgin-2.10.11/libpurple

# Compiler
CC := i686-w64-mingw32-gcc
CFLAGS = -O2 -Wall -Waggregate-return -Wcast-align -Wdeclaration-after-statement -Werror-implicit-function-declaration -Wextra -Wno-sign-compare -Wno-unused-parameter -Winit-self -Wmissing-declarations -Wmissing-prototypes -Wnested-externs -Wpointer-arith -Wundef -pipe -mms-bitfields -g
LDFLAGS = -Wl,--enable-auto-image-base -Wl,--enable-auto-import -Wl,--dynamicbase -Wl,--nxcompat


INCLUDE_PATHS +=	\
			-I$(GTK_TOP)/include \
			-I$(GTK_TOP)/include/glib-2.0 \
			-I$(GTK_TOP)/lib/glib-2.0/include \
			-I$(PURPLE_TOP)

LIB_PATHS = -L`pwd` -L$(GTK_TOP)/lib

LIBS = -lglib-2.0 -lpurple


.SUFFIXES: .c .dll
.PHONY: all clean

all: $(TARGET)

.c.dll:
	$(CC) $(CFLAGS) $(INCLUDE_PATHS) -mwindows -o $@.o -c $<
	$(CC) -shared $@.o $(LIB_PATHS) $(LIBS) $(LD_FLAGS) -o $@
	rm $@.o

clean:
	rm -f *.o *.dll
