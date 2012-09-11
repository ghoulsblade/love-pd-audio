UNAME = $(shell uname)
SOLIB_PREFIX = lib

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
    LDFLAGS = $(MINGW_LDFLAGS) -Wl,--output-def=lovepdaudio.def -Wl,--out-implib=lovepdaudio.lib
  else  # Assume Linux
    CC = g++
    LUA_INC= /usr/include/lua5.1
    SOLIB_EXT = so
    PLATFORM_CFLAGS = -DHAVE_LIBDL -fPIC -O3 -I$(LUA_INC)
    LDFLAGS = -shared -ldl -Wl,-Bsymbolic -Llib lib/libpd.so -lopenal
    AR= ar rcu
    RANLIB= ranlib
  endif
endif

MY_FILES = \
	lovepdaudio.cpp 


LIBNAME = lovepdaudio.$(SOLIB_EXT)

CFLAGS = -DHAVE_UNISTD_H -DUSEAPI_DUMMY -I./include $(PLATFORM_CFLAGS)
CXXFLAGS = $(CFLAGS)

.PHONY: lovepdaudio clean clobber


lovepdaudio: $(LIBNAME)

$(LIBNAME): ${MY_FILES:.cpp=.o}
	$(CC) -o $(LIBNAME) $^ $(LDFLAGS) -lm 

# TODO: what is -lm ? taken from libpd Makefile, might not be necessary

clean:
	rm -f ${MY_FILES:.cpp=.o}

clobber: clean
	rm -f $(LIBNAME)



# LUA_INC= /usr/include/lua5.1
# WARN= 
# INCS= -I$(LUA_INC) -Iinclude
# LIBPD = libs/libpd.so
# MYFLAGS= -fPIC   
# CFLAGS= -O2 $(WARN) $(INCS) $(DEFS) $(MYFLAGS)
# CXXFLAGS= -O2 $(WARN) $(INCS) $(DEFS) $(MYFLAGS)
# CC= g++
# LIB_OPTION= -shared -lopenal -Llibs $(MYFLAGS) #for Linux
# LIBNAME= lovepdaudio.so
# OBJS= lovepdaudio.o
# SRCS= lovepdaudio.cpp
# AR= ar rcu
# RANLIB= ranlib
# lib: $(LIBNAME)
# $(LIBNAME): $(OBJS)
# 	$(CC) $(CFLAGS) -o $@ $(LIB_OPTION) $(LIBPD) $(OBJS)
