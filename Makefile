LUA_INC= /usr/include/lua5.1

#~ WARN= -Wall -Wmissing-prototypes -Wmissing-declarations -ansi -pedantic
WARN= 
INCS= -I$(LUA_INC) -Iinclude

#~ LIBPD =  # no libpd test
LIBPD = libs/libpd.so

#~ MYFLAGS= -fPIC : 64bit only?
#~ MYFLAGS= -m32 : compile for 32 bit
MYFLAGS= -fPIC   

CFLAGS= -O2 $(WARN) $(INCS) $(DEFS) $(MYFLAGS)
CXXFLAGS= -O2 $(WARN) $(INCS) $(DEFS) $(MYFLAGS)
CC= g++

# OS dependent
#~ LIB_OPTION= -shared -Llibs $(MYFLAGS) #for Linux  no-audio-test
LIB_OPTION= -shared -lopenal -Llibs $(MYFLAGS) #for Linux
#LIB_OPTION= -bundle -undefined dynamic_lookup #for MacOS X

LIBNAME= lovepdaudio.so

OBJS= lovepdaudio.o
SRCS= lovepdaudio.cpp
AR= ar rcu
RANLIB= ranlib

lib: $(LIBNAME)

$(LIBNAME): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(LIB_OPTION) $(LIBPD) $(OBJS)
	
#~ export MACOSX_DEPLOYMENT_TARGET="10.3"; $(CC) $(CFLAGS) -o $@ $(LIB_OPTION) $(OBJS)

#~ $(COMPAT_DIR)/compat-5.1.o: $(COMPAT_DIR)/compat-5.1.c
#~ $(CC) -c $(CFLAGS) -o $@ $(COMPAT_DIR)/compat-5.1.c

#~ install:
#~ mkdir -p $(LUA_LIBDIR)/lovepdaudio
#~ cp src/$(LIBNAME) $(LUA_LIBDIR)/lovepdaudio

clean:
	rm -f $(LIBNAME) *.o
