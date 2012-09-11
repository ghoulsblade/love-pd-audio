UNAME = $(shell uname)
SOLIB_PREFIX = lib

# note: building 32 bit binary on 64 bit machine : ./configure --build=i686-pc-linux-gnu "CFLAGS=-m32" "CXXFLAGS=-m32" "LDFLAGS=-m32"
MY_COMPILE_AND_LINK_FLAGS =
#~ MY_COMPILE_AND_LINK_FLAGS = -m32
#~ MY_COMPILE_AND_LINK_FLAGS = -m64

# note: static lib : http://stackoverflow.com/questions/2734719/howto-compile-a-static-library-in-linux

ifeq ($(UNAME), Darwin)  # Mac
  SOLIB_EXT = dylib
  PLATFORM_CFLAGS = -DHAVE_LIBDL -O3 -arch x86_64 -arch i386 -g
  LDFLAGS = -arch x86_64 -arch i386 -dynamiclib -ldl
else
  ifeq ($(OS), Windows_NT)  # Windows, use Mingw
    CC = g++
    SOLIB_EXT = dll
    SOLIB_PREFIX = 
    PLATFORM_CFLAGS = -DWINVER=0x502 -DWIN32 -D_WIN32 -O3 -static-libgcc -static-libstdc++
    MINGW_LDFLAGS = -shared -lkernel32 -Llib -llua -llua5.1 -lopenal32 -llibpd -static-libgcc -static-libstdc++
    LDFLAGS = $(MINGW_LDFLAGS) -Wl,--output-def=bin/lovepdaudio.def -Wl,--out-implib=bin/lovepdaudio.lib
  else  # Assume Linux
    CC = g++
    LUA_INC= /usr/include/lua5.1
    SOLIB_EXT = so
    PLATFORM_CFLAGS = -DHAVE_LIBDL -fPIC -O3 -I$(LUA_INC) $(MY_COMPILE_AND_LINK_FLAGS)
    LDFLAGS = -shared -ldl -Wl,-Bsymbolic -Llib -Llib64 -lpd -lopenal $(MY_COMPILE_AND_LINK_FLAGS)
    AR= ar rcu
    RANLIB= ranlib
  endif
endif

MY_FILES = \
	lovepdaudio.cpp 


LIBNAME = bin/lovepdaudio.$(SOLIB_EXT)

CFLAGS = -DHAVE_UNISTD_H -DUSEAPI_DUMMY -I./include $(PLATFORM_CFLAGS)
CXXFLAGS = $(CFLAGS)

.PHONY: lovepdaudio clean clobber


lovepdaudio: $(LIBNAME)

$(LIBNAME): ${MY_FILES:.cpp=.o}
	$(CC) -o $(LIBNAME) $^ $(LDFLAGS) -lm 

# TODO: from libpd makefile, not needed anymore ?  CFLAGS: -DHAVE_UNISTD_H -DUSEAPI_DUMMY     win PLATFORM_CFLAGS :  -DWINVER=0x502   -DWIN32 -D_WIN32       mac+lin:-DHAVE_LIBDL
# TODO: what is -lm ? taken from libpd Makefile, might not be necessary

clean:
	rm -f ${MY_FILES:.cpp=.o}

clobber: clean
	rm -f $(LIBNAME)
